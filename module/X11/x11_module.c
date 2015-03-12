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
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#endif

#include <winpr/pipe.h>
#include <winpr/path.h>
#include <winpr/shell.h>
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/wlog.h>
#include <winpr/environment.h>

#include <freerds/backend.h>

#include "x11_module.h"

RDS_MODULE_CONFIG_CALLBACKS g_Config;
RDS_MODULE_STATUS_CALLBACKS g_Status;

#define X11_DISPLAY_OFFSET 10
#define X11_LOCKFILE_FORMAT "/tmp/.X%d-lock"
#define X11_UNIX_SOCKET_FORMAT "/tmp/.X11-unix/X%d"
#define X11_DISPLAY_MAX 1024

static wLog* gModuleLog;

struct rds_module_x11
{
	RDS_MODULE_COMMON commonModule;

	STARTUPINFO X11StartupInfo;
	PROCESS_INFORMATION X11ProcessInformation;

	HANDLE monitorThread;
	HANDLE monitorStopEvent;
	STARTUPINFO CSStartupInfo;
	PROCESS_INFORMATION CSProcessInformation;
	STARTUPINFO WMStartupInfo;
	PROCESS_INFORMATION WMProcessInformation;
	UINT32 displayNum;
};
typedef struct rds_module_x11 rdsModuleX11;

void x11_rds_module_reset_process_informations(STARTUPINFO* si, PROCESS_INFORMATION* pi)
{
	ZeroMemory(si, sizeof(STARTUPINFO));
	si->cb = sizeof(STARTUPINFO);
	ZeroMemory(pi, sizeof(PROCESS_INFORMATION));
}

int clean_up_process(PROCESS_INFORMATION* pi)
{
	DWORD ret = 0;

	if (pi->hProcess)
	{
		GetExitCodeProcess(pi->hProcess, &ret);
		CloseHandle(pi->hProcess);
		pi->hProcess = NULL;
	}
	if (pi->hThread)
	{
		CloseHandle(pi->hThread);
		pi->hThread = NULL;
	}

	return ret;
}

void monitoring_thread(void* arg)
{
	int status;
	int ret;
	rdsModuleX11* x11 = (rdsModuleX11*) arg;
	
	while (1)
	{
		if (waitpid(x11->X11ProcessInformation.dwProcessId, &status, WNOHANG) != 0)
		{
			ret = clean_up_process(&(x11->X11ProcessInformation));
			WLog_Print(gModuleLog, WLOG_DEBUG, "s %d: X11 process exited with %d (monitoring thread)", x11->commonModule.sessionId, ret);
			break;
		}
		if (waitpid(x11->CSProcessInformation.dwProcessId, &status, WNOHANG) != 0)
		{
			ret = clean_up_process(&(x11->CSProcessInformation));
			WLog_Print(gModuleLog, WLOG_DEBUG, "s %d: CS process exited with %d (monitoring thread)", x11->commonModule.sessionId, ret);
			break;
		}
		if (waitpid(x11->WMProcessInformation.dwProcessId, &status, WNOHANG) != 0)
		{
			ret = clean_up_process(&(x11->WMProcessInformation));
			WLog_Print(gModuleLog, WLOG_DEBUG, "s %d: WM process exited with %d (monitoring thread)", x11->commonModule.sessionId, ret);
			break;
		}
		if (WaitForSingleObject(x11->monitorStopEvent, 200) == WAIT_OBJECT_0)
		{
			// monitorStopEvent triggered
			WLog_Print(gModuleLog, WLOG_DEBUG, "s %d: monitor stop event", x11->commonModule.sessionId);
			return;
		}
	}

	if (x11->CSProcessInformation.hProcess)
	{
		kill(x11->CSProcessInformation.dwProcessId, SIGTERM);
		if (waitpid(x11->CSProcessInformation.dwProcessId, &status, 0) != 0)
		{
			ret = clean_up_process(&(x11->CSProcessInformation));
			WLog_Print(gModuleLog, WLOG_DEBUG, "s %d: CS process exited with %d (monitoring thread)", x11->commonModule.sessionId, ret);
		}
	}

	g_Status.shutdown(x11->commonModule.sessionId);
	return;
}

RDS_MODULE_COMMON* x11_rds_module_new(void)
{
	rdsModuleX11* module = (rdsModuleX11*) calloc(1, sizeof(rdsModuleX11));
	return (RDS_MODULE_COMMON*) module;
}

void x11_rds_module_free(RDS_MODULE_COMMON* module)
{
	rdsModuleX11* moduleCon = (rdsModuleX11*) module;

	if (moduleCon->commonModule.authToken)
		free(moduleCon->commonModule.authToken);

	if (moduleCon->commonModule.baseConfigPath)
		free(moduleCon->commonModule.baseConfigPath);

	free(module);
}

int x11_rds_stop_process(PROCESS_INFORMATION *pi)
{
	int ret = 0, status = 0;
	int wait = 10;

	/* check if child is still alive */
	status = waitpid(pi->dwProcessId, &ret, WNOHANG);

	if (status == 0)
	{
		kill(pi->dwProcessId, SIGTERM);

		while (wait > 0)
		{
			status = waitpid(pi->dwProcessId, &ret, WNOHANG);

			if (status != 0)
				break;

			usleep(500000);
			wait--;
		}

		if (status == 0)
			kill(pi->dwProcessId, SIGKILL);
	}
	clean_up_process(pi);

	return ret;
}

unsigned int detect_free_display()
{
	struct stat tstats;
	unsigned int i = 0;
	char buf[256];
	char buf2[256];

	for (i = X11_DISPLAY_OFFSET; i <= X11_DISPLAY_MAX; i++)
	{
		snprintf(buf,256, X11_LOCKFILE_FORMAT, i);
		snprintf(buf2,256, X11_UNIX_SOCKET_FORMAT, i);

		if(stat (buf, &tstats) != 0 && stat(buf2, &tstats) != 0)
		{
			break;
		}
	}

	return i;
}

char* x11_rds_module_start(RDS_MODULE_COMMON* module)
{
	static const char* envNames[] = {
		"WLOG_APPENDER",
		"WLOG_FILTER",
		"WLOG_LEVEL",
		"WLOG_FILEAPPENDER_OUTPUT_FILE_PATH",
		"WLOG_FILEAPPENDER_OUTPUT_FILE_NAME"
	};

	BOOL status = TRUE;
	DWORD SessionId;
	char envstr[256];
	char* envstrp;
	char* filename;
	char* pipeName;
	rdsModuleX11* x11;
	char commandLine[256];
	char currentDir[256];
	char* lpCurrentDir;
	char startupname[256];
	DWORD cchSize;
	int i;

	x11 = (rdsModuleX11*) module;
	x11->monitorStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	SessionId = x11->commonModule.sessionId;
	x11->displayNum = detect_free_display();

	WLog_Print(gModuleLog, WLOG_DEBUG, "s %d, using display %d", SessionId, x11->displayNum);
	pipeName = (char*) malloc(256);
	freerds_named_pipe_get_endpoint_name(x11->displayNum, "X11", pipeName, 256);

	filename = GetNamedPipeUnixDomainSocketFilePathA(pipeName);

	if (PathFileExistsA(filename))
	{
		DeleteFileA(filename);
		status = 1;
	}

	free(filename);

	sprintf_s(envstr, sizeof(envstr), ":%d", (int) (x11->displayNum));
	SetEnvironmentVariableEBA(&x11->commonModule.envBlock, "DISPLAY", envstr);

	sprintf_s(commandLine, sizeof(commandLine), "%s :%d -geometry %dx%d -depth %d -dpi 96",
			"Xrds", (int) (x11->displayNum), module->desktopWidth, module->desktopHeight, 24);

	/* Get the user's home directory. */
	lpCurrentDir = NULL;
	cchSize = sizeof(currentDir);
	ZeroMemory(currentDir, sizeof(currentDir));
	if (GetUserProfileDirectoryA(x11->commonModule.userToken, currentDir, &cchSize))
	{
		WLog_Print(gModuleLog, WLOG_DEBUG, "HOME=%s", currentDir);
		lpCurrentDir = currentDir;
	}

	/* Propagate environment variables. */
	for (i = 0; i < (sizeof(envNames) / sizeof(envNames[0])); i++)
	{
		const char* envname = envNames[i];

		envstrp = getenv(envname);
		if (envstrp)
		{
			WLog_Print(gModuleLog, WLOG_DEBUG, "%s=%s", envname, envstrp);
			SetEnvironmentVariableEBA(&x11->commonModule.envBlock, envname, envstrp);
		}
	}

	x11_rds_module_reset_process_informations(&(x11->X11StartupInfo), &(x11->X11ProcessInformation));

	status = CreateProcessAsUserA(x11->commonModule.userToken, NULL, commandLine,
			NULL, NULL, FALSE, 0, x11->commonModule.envBlock, NULL,
			&(x11->X11StartupInfo), &(x11->X11ProcessInformation));

	if (!status)
	{
		WLog_Print(gModuleLog, WLOG_ERROR , "s %d, problem starting Xrds (status %d - cmd %s)",
		SessionId, status, commandLine);
		free(pipeName);
		return NULL;
	}

	WLog_Print(gModuleLog, WLOG_DEBUG, "s %d, Xrds Process started: %d (pid %d - cmd %s)",
			SessionId, status, x11->X11ProcessInformation.dwProcessId, commandLine);

	if (!WaitNamedPipeA(pipeName, 5 * 1000))
	{
		WLog_Print(gModuleLog, WLOG_ERROR, "s %d: WaitNamedPipe failure: %s\n", SessionId, pipeName);
		free(pipeName);
		return NULL;
	}

	x11_rds_module_reset_process_informations(&(x11->CSStartupInfo), &(x11->CSProcessInformation));
	x11_rds_module_reset_process_informations(&(x11->WMStartupInfo), &(x11->WMProcessInformation));

	sprintf_s(envstr, sizeof(envstr), "%d", (int) (x11->commonModule.sessionId));
	SetEnvironmentVariableEBA(&x11->commonModule.envBlock, "FREERDS_SID", envstr);

	/* Start the FreeRDS channel server. */
	strcpy(startupname, "freerds-channels");

	status = CreateProcessAsUserA(x11->commonModule.userToken, NULL, startupname,
		NULL, NULL, FALSE, 0, x11->commonModule.envBlock, NULL,
		&(x11->CSStartupInfo), &(x11->CSProcessInformation));

	if (!status)
	{
		WLog_Print(gModuleLog, WLOG_DEBUG, "s %d, problem starting %s (status %d)", SessionId, startupname, status);
		x11_rds_stop_process(&(x11->X11ProcessInformation));
		free(pipeName);
		return NULL;
	}

	WLog_Print(gModuleLog, WLOG_DEBUG, "s %d: CS process started: %d (pid %d)", SessionId, status, x11->CSProcessInformation.dwProcessId);

	/* Start the window manager. */
	if (x11->commonModule.userToken == 0)
	{
		strcpy(startupname, "simple_greeter");
	}
	else
	{
		if (!getPropertyStringWrapper(x11->commonModule.baseConfigPath, &g_Config, "startwm", startupname, 256))
		{
			strcpy(startupname, "startwm.sh");
		}
	}

	status = CreateProcessAsUserA(x11->commonModule.userToken, NULL, startupname,
		NULL, NULL, FALSE, 0, x11->commonModule.envBlock, lpCurrentDir,
		&(x11->WMStartupInfo), &(x11->WMProcessInformation));

	if (!status)
	{
		WLog_Print(gModuleLog, WLOG_DEBUG, "s %d, problem starting %s (status %d)", SessionId, startupname, status);
		x11_rds_stop_process(&(x11->X11ProcessInformation));
		x11_rds_stop_process(&(x11->CSProcessInformation));
		free(pipeName);
		return NULL;
	}

	WLog_Print(gModuleLog, WLOG_DEBUG, "s %d: WM process started: %d (pid %d)", SessionId, status, x11->WMProcessInformation.dwProcessId);

	/* Start the monitoring thread. */
	x11->monitorThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) monitoring_thread, x11, 0, NULL);

	return pipeName;
}

int x11_rds_module_stop(RDS_MODULE_COMMON* module)
{
	int ret = 0;
	char buf[256];
	rdsModuleX11* x11 = (rdsModuleX11*) module;

	WLog_Print(gModuleLog, WLOG_TRACE, "Stop called");

	SetEvent(x11->monitorStopEvent);
	WaitForSingleObject(x11->monitorThread, INFINITE);

	ret = x11_rds_stop_process(&(x11->WMProcessInformation));
	ret = x11_rds_stop_process(&(x11->CSProcessInformation));
	ret = x11_rds_stop_process(&(x11->X11ProcessInformation));

	/* clean up in case x server wasn't shut down cleanly */

	snprintf(buf,256, X11_LOCKFILE_FORMAT, x11->displayNum);
	DeleteFileA(buf);

	snprintf(buf,256, X11_UNIX_SOCKET_FORMAT, x11->displayNum);
	DeleteFileA(buf);

	return ret;
}

int RdsModuleEntry(RDS_MODULE_ENTRY_POINTS* pEntryPoints)
{
	pEntryPoints->Version = 1;

	pEntryPoints->New = x11_rds_module_new;
	pEntryPoints->Free = x11_rds_module_free;

	pEntryPoints->Start = x11_rds_module_start;
	pEntryPoints->Stop = x11_rds_module_stop;

	pEntryPoints->Name = "X11";

	g_Status = pEntryPoints->status;
	g_Config = pEntryPoints->config;

	WLog_Init();
	gModuleLog = WLog_Get("com.freerds.module.x11");

	return 0;
}
