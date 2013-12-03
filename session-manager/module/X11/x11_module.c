/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * X11 Server Module
 *
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright 2013 Thincast Technologies GmbH
 * Copyright 2013 DI (FH) Martin Haimberger <martin.haimberger@thincast.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef WIN32
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <winpr/pipe.h>
#include <winpr/path.h>
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/wlog.h>
#include <winpr/environment.h>

#include <freerds/backend.h>

#include "x11_module.h"

RDS_MODULE_CONFIG_CALLBACKS gConfig;
RDS_MODULE_STATUS_CALLBACKS gStatus;

static wLog *gModuleLog;

struct rds_module_x11
{
	RDS_MODULE_COMMON commonModule;

	STARTUPINFO X11StartupInfo;
	PROCESS_INFORMATION X11ProcessInformation;

	HANDLE monitorThread;
	HANDLE monitorStopEvent;
	STARTUPINFO WMStartupInfo;
	PROCESS_INFORMATION WMProcessInformation;
	BOOL wmRunning;
};

typedef struct rds_module_x11 rdsModuleX11;

void x11_rds_module_reset_process_informations(STARTUPINFO *si, PROCESS_INFORMATION *pi)
{
	ZeroMemory(si, sizeof(STARTUPINFO));
	si->cb = sizeof(STARTUPINFO);
	ZeroMemory(pi, sizeof(PROCESS_INFORMATION));
}

void monitoring_thread(void *arg)
{
	DWORD ret = 0;
	int status;
	rdsModuleX11 *x11 = (rdsModuleX11*)arg;
	
	while (1)
	{
		ret = waitpid(x11->WMProcessInformation.dwProcessId, &status, WNOHANG);
		if (ret != 0)
		{
			break;
		}
		if (WaitForSingleObject(x11->monitorStopEvent, 200) == WAIT_OBJECT_0)
		{
			// monitorStopEvent triggered
			WLog_Print(gModuleLog, WLOG_DEBUG, "s %d: monitor stop event", x11->commonModule.sessionId);
			return;
		}
	}

	x11->wmRunning = FALSE;
	GetExitCodeProcess(x11->WMProcessInformation.hProcess, &ret);
	CloseHandle(x11->WMProcessInformation.hProcess);
	CloseHandle(x11->WMProcessInformation.hThread);
	WLog_Print(gModuleLog, WLOG_DEBUG, "s %d: WM process exited with %d (monitoring thread)", x11->commonModule.sessionId, ret);
	gStatus.shutdown(x11->commonModule.sessionId);
	return;
}

RDS_MODULE_COMMON* x11_rds_module_new(void)
{
	rdsModuleX11* module = (rdsModuleX11*) malloc(sizeof(rdsModuleX11));
	ZeroMemory(module, sizeof(rdsModuleX11));
	return (RDS_MODULE_COMMON*) module;
}

void x11_rds_module_free(RDS_MODULE_COMMON* module)
{
	rdsModuleX11* moduleCon = (rdsModuleX11*) module;

	if (moduleCon->commonModule.authToken)
		free(moduleCon->commonModule.authToken);

	free(module);
}

int x11_rds_stop_process(PROCESS_INFORMATION *pi)
{
#ifdef WIN32
	TerminateProcess(pi.hProcess,0);

	 // Wait until child process exits.
	WaitForSingleObject(pi->hProcess, 5);
	GetExitCodeProcess(pi->hProcess, &ret);
#else
	int ret = 0, status = 0;
	int wait = 10;
	kill(pi->dwProcessId, SIGTERM);
	while (wait > 0)
	{
		status = waitpid(pi->dwProcessId, &ret, WNOHANG);
		if (status == pi->dwProcessId)
			break;
		usleep(500000);
		wait--;
	}
	if (!status)
		kill(pi->dwProcessId, SIGKILL);
#endif
	CloseHandle(pi->hProcess);
	CloseHandle(pi->hThread);
	return ret;
}

void initResolutions(rdsModuleX11 * x11,  long * xres, long * yres, long * colordepth) {
	char tempstr[256];

	long maxXRes, maxYRes, minXRes, minYRes = 0;
	long connectionXRes, connectionYRes, connectionColorDepth = 0;

	if (!gConfig.getPropertyNumber(x11->commonModule.sessionId, "module.x11.maxXRes", &maxXRes)) {
		WLog_Print(gModuleLog, WLOG_ERROR, "Setting: module.x11.maxXRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n");
	}
	if (!gConfig.getPropertyNumber(x11->commonModule.sessionId, "module.x11.maxYRes", &maxYRes)) {
		WLog_Print(gModuleLog, WLOG_ERROR, "Setting: module.x11.maxYRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n");
	}
	if (!gConfig.getPropertyNumber(x11->commonModule.sessionId, "module.x11.minXRes", &minXRes)) {
		WLog_Print(gModuleLog, WLOG_ERROR, "Setting: module.x11.minXRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n");
	}
	if (!gConfig.getPropertyNumber(x11->commonModule.sessionId, "module.x11.minYRes", &minYRes)){
		WLog_Print(gModuleLog, WLOG_ERROR, "Setting: module.x11.minYRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n");
	}

	if ((maxXRes != 0) && (maxYRes != 0) && (minXRes != 0) && (minYRes != 0)) {
		sprintf_s(tempstr, sizeof(tempstr), "%dx%d", (unsigned int) maxXRes,(unsigned int) maxYRes );
		SetEnvironmentVariableEBA(&x11->commonModule.envBlock, "FREERDS_SMAX", tempstr);

		sprintf_s(tempstr, sizeof(tempstr), "%dx%d", (unsigned int) minXRes,(unsigned int) minYRes );
		SetEnvironmentVariableEBA(&x11->commonModule.envBlock, "FREERDS_SMIN", tempstr);
	}

	gConfig.getPropertyNumber(x11->commonModule.sessionId, "connection.xres", &connectionXRes);
	gConfig.getPropertyNumber(x11->commonModule.sessionId, "connection.yres", &connectionYRes);
	gConfig.getPropertyNumber(x11->commonModule.sessionId, "connection.colordepth", &connectionColorDepth);

	if ((connectionXRes == 0) || (connectionYRes == 0)) {
		WLog_Print(gModuleLog, WLOG_ERROR, "got no XRes or YRes from client, using config values");

		if (!gConfig.getPropertyNumber(x11->commonModule.sessionId, "module.x11.xres", xres))
			*xres = 1024;

		if (!gConfig.getPropertyNumber(x11->commonModule.sessionId, "module.x11.yres", yres))
			*yres = 768;

		if (!gConfig.getPropertyNumber(x11->commonModule.sessionId, "module.x11.colordepth", colordepth))
			*colordepth = 24;
		return;
	}

	if ((maxXRes > 0 ) && (connectionXRes > maxXRes)) {
		*xres = maxXRes;
	} else if ((minXRes > 0 ) && (connectionXRes < minXRes)) {
		*xres = minXRes;
	} else {
		*xres = connectionXRes;
	}

	if ((maxYRes > 0 ) && (connectionYRes > maxYRes)) {
		*yres = maxYRes;
	} else if ((minYRes > 0 ) && (connectionYRes < minYRes)) {
		*yres = minYRes;
	} else {
		*yres = connectionYRes;
	}
	if (connectionColorDepth == 0) {
		connectionColorDepth = 16;
	}
	*colordepth = connectionColorDepth;
}

char* x11_rds_module_start(RDS_MODULE_COMMON * module)
{
	BOOL status = TRUE;
	DWORD SessionId;
	DWORD displayNum;
	char envstr[256];
	rdsModuleX11* x11;
	char lpCommandLine[256];
	char startupname[256];
	char* filename;
	char* pipeName;
	long xres, yres, colordepth;

	x11 = (rdsModuleX11*) module;
	x11->monitorStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	SessionId = x11->commonModule.sessionId;
	displayNum = SessionId + 10;

	pipeName = (char*) malloc(256);
	freerds_named_pipe_get_endpoint_name(displayNum, "X11", pipeName, 256);

	filename = GetNamedPipeUnixDomainSocketFilePathA(pipeName);

	if (PathFileExistsA(filename))
	{
		DeleteFileA(filename);
		status = 1;
	}

	free(filename);

	sprintf_s(envstr, sizeof(envstr), ":%d", (int) (displayNum));
	SetEnvironmentVariableEBA(&x11->commonModule.envBlock, "DISPLAY", envstr);

	initResolutions(x11,&xres,&yres,&colordepth);

	sprintf_s(lpCommandLine, sizeof(lpCommandLine), "%s :%d -geometry %dx%d -depth %d -uds -terminate",
			"X11rdp", (int) (displayNum), (int) xres, (int) yres, (int) colordepth);

	x11_rds_module_reset_process_informations(&(x11->X11StartupInfo), &(x11->X11ProcessInformation));
	status = CreateProcessA(NULL, lpCommandLine,
			NULL, NULL, FALSE, 0, x11->commonModule.envBlock, NULL,
			&(x11->X11StartupInfo), &(x11->X11ProcessInformation));

	if (!status)
	{
		WLog_Print(gModuleLog, WLOG_DEBUG, "s %d, problem startin X11rdp (status %d)\n", SessionId, status);
		return NULL;
	}
	x11->wmRunning = TRUE;

	WLog_Print(gModuleLog, WLOG_DEBUG, "s %d, X11rdp Process started: %d (pid %d)\n", SessionId, status, x11->X11ProcessInformation.dwProcessId);

	if (!WaitNamedPipeA(pipeName, 5 * 1000))
	{
		WLog_Print(gModuleLog, WLOG_ERROR, "s %d: WaitNamedPipe failure: %s\n", SessionId, pipeName);
		return NULL;
	}

	x11_rds_module_reset_process_informations(&(x11->WMStartupInfo), &(x11->WMProcessInformation));

	sprintf_s(envstr, sizeof(envstr), "%d", (int) (x11->commonModule.sessionId));
	SetEnvironmentVariableEBA(&x11->commonModule.envBlock, "FREERDS_SID", envstr);

	if (!gConfig.getPropertyString(x11->commonModule.sessionId, "module.x11.startwm", startupname, 256))
		strcpy(startupname, "startwm.sh");

	status = CreateProcessAsUserA(x11->commonModule.userToken,
			NULL, startupname,
			NULL, NULL, FALSE, 0, x11->commonModule.envBlock, NULL,
			&(x11->WMStartupInfo), &(x11->WMProcessInformation));

	WLog_Print(gModuleLog, WLOG_DEBUG, "s %d: WM process started: %d (pid %d)", SessionId, status, x11->WMProcessInformation.dwProcessId);
	x11->monitorThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) monitoring_thread, x11, 0, NULL);

	return pipeName;
}

int x11_rds_module_stop(RDS_MODULE_COMMON * module)
{
	rdsModuleX11 *x11 = (rdsModuleX11*)module;
	int ret;
	WLog_Print(gModuleLog, WLOG_TRACE, "Stop called");

	SetEvent(x11->monitorStopEvent);
	WaitForSingleObject(x11->monitorThread, INFINITE);

	if (x11->wmRunning)
	{
		x11_rds_stop_process(&(x11->WMProcessInformation));
	}

	ret = x11_rds_stop_process(&(x11->X11ProcessInformation));
	return ret;
}

int RdsModuleEntry(RDS_MODULE_ENTRY_POINTS* pEntryPoints)
{
	pEntryPoints->Version = 1;

	//pEntryPoints->ContextSize = sizeof(rdsModuleX11);
	pEntryPoints->New = x11_rds_module_new;
	pEntryPoints->Free = x11_rds_module_free;

	pEntryPoints->Start = x11_rds_module_start;
	pEntryPoints->Stop = x11_rds_module_stop;

	pEntryPoints->Name = "X11";

	gStatus = pEntryPoints->status;
	gConfig = pEntryPoints->config;

	WLog_Init();
	gModuleLog = WLog_Get("com.freerds.module.x11");
	return 0;
}
