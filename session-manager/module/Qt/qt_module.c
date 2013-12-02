/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * Qt Server Module
 *
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright 2013 Thinstuff Technologies GmbH
 * Copyright 2013 DI (FH) Martin Haimberger <martin.haimberger@thinstuff.at>
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

#include <winpr/crt.h>
#include <winpr/wlog.h>
#include <winpr/pipe.h>
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/environment.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <freerds/module.h>
#include <freerds/backend.h>

#include "qt_module.h"

RDS_MODULE_CONFIG_CALLBACKS gConfig;
RDS_MODULE_STATUS_CALLBACKS gStatus;

struct rds_module_qt
{
	RDS_MODULE_COMMON commonModule;

	wLog* log;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
};
typedef struct rds_module_qt rdsModuleQt;


RDS_MODULE_COMMON* qt_rds_module_new(void)
{
	rdsModuleQt* qt = (rdsModuleQt*) malloc(sizeof(rdsModuleQt));

	WLog_Init();

	qt->log = WLog_Get("com.freerds.module.qt");
	WLog_OpenAppender(qt->log);

	WLog_SetLogLevel(qt->log, WLOG_DEBUG);

	WLog_Print(qt->log, WLOG_DEBUG, "RdsModuleNew");

	return (RDS_MODULE_COMMON*) qt;
}

void qt_rds_module_free(RDS_MODULE_COMMON* module)
{
	rdsModuleQt* qt = (rdsModuleQt*) module;
	WLog_Print(qt->log, WLOG_DEBUG, "RdsModuleFree");
	WLog_Uninit();
	free(module);
}

void initMaxMinRes(rdsModuleQt * qt) {
	char tempstr[256];

	long maxXRes, maxYRes, minXRes, minYRes = 0;

	if (!gConfig.getPropertyNumber(qt->commonModule.sessionId, "module.qt.maxXRes", &maxXRes)) {
		WLog_Print(qt->log, WLOG_ERROR, "Setting: module.qt.maxXRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n");
	}
	if (!gConfig.getPropertyNumber(qt->commonModule.sessionId, "module.qt.maxYRes", &maxYRes)) {
		WLog_Print(qt->log, WLOG_ERROR, "Setting: module.qt.maxYRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n");
	}
	if (!gConfig.getPropertyNumber(qt->commonModule.sessionId, "module.qt.minXRes", &minXRes)) {
		WLog_Print(qt->log, WLOG_ERROR, "Setting: module.qt.minXRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n");
	}
	if (!gConfig.getPropertyNumber(qt->commonModule.sessionId, "module.qt.minYRes", &minYRes)){
		WLog_Print(qt->log, WLOG_ERROR, "Setting: module.qt.minYRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n");
	}

	if ((maxXRes != 0) && (maxYRes != 0) && (minXRes != 0) && (minYRes != 0)) {
		sprintf_s(tempstr, sizeof(tempstr), "%dx%d", (unsigned int) maxXRes,(unsigned int) maxYRes );
		SetEnvironmentVariableEBA(&qt->commonModule.envBlock, "FREERDS_SMAX", tempstr);

		sprintf_s(tempstr, sizeof(tempstr), "%dx%d", (unsigned int) minXRes,(unsigned int) minYRes );
		SetEnvironmentVariableEBA(&qt->commonModule.envBlock, "FREERDS_SMIN", tempstr);
	}
}

char* qt_rds_module_start(RDS_MODULE_COMMON* module)
{
	BOOL status;
	char* pipeName;
	char* qPipeName;
	long xres, yres;
	char lpCommandLine[256];
	const char* endpoint = "Qt";
	char envstr[256];

	rdsModuleQt* qt = (rdsModuleQt*) module;
	DWORD SessionId = qt->commonModule.sessionId;
	char *appName = "nice_greeter";

	WLog_Print(qt->log, WLOG_DEBUG, "RdsModuleStart: SessionId: %d Endpoint: %s",
			(int) SessionId, endpoint);

	pipeName = (char*) malloc(256);
	freerds_named_pipe_get_endpoint_name(SessionId, endpoint, pipeName, 256);
	freerds_named_pipe_clean_endpoint(SessionId, endpoint);

	ZeroMemory(&(qt->si), sizeof(STARTUPINFO));
	qt->si.cb = sizeof(STARTUPINFO);
	ZeroMemory(&(qt->pi), sizeof(PROCESS_INFORMATION));

	sprintf_s(envstr, sizeof(envstr), "%d", (int) (qt->commonModule.sessionId));
	SetEnvironmentVariableEBA(&qt->commonModule.envBlock, "FREERDS_SID", envstr);


	if (!gConfig.getPropertyNumber(qt->commonModule.sessionId, "module.qt.xres", &xres))
		xres = 1024;

	if (!gConfig.getPropertyNumber(qt->commonModule.sessionId, "module.qt.yres", &yres))
		yres = 768;

	initMaxMinRes(qt);

	qPipeName = (char*) malloc(256);
	sprintf_s(qPipeName, 256, "/tmp/.pipe/FreeRDS_%d_%s", (int) SessionId, endpoint);
	SetEnvironmentVariableEBA(&qt->commonModule.envBlock, "FREERDS_PIPE_PATH", qPipeName);
	free(qPipeName);

	SetEnvironmentVariableEBA(&qt->commonModule.envBlock, "QT_PLUGIN_PATH",
			"/opt/freerds/lib64/plugins");

	sprintf_s(lpCommandLine, sizeof(lpCommandLine), "%s -platform freerds",
			appName);

	WLog_Print(qt->log, WLOG_DEBUG, "Starting process with command line: %s", lpCommandLine);

	status = CreateProcessA(NULL, lpCommandLine,
			NULL, NULL, FALSE, 0, qt->commonModule.envBlock, NULL,
			&(qt->si), &(qt->pi));
	if (0 == status)
	{
		WLog_Print(qt->log, WLOG_ERROR, "Could not start qt application %s", appName);
		return NULL;
	}

	WLog_Print(qt->log, WLOG_DEBUG, "Process %d/%d created with status: %d", qt->pi.dwProcessId,qt->pi.dwThreadId, status);

	if (!WaitNamedPipeA(pipeName, 5 * 1000))
	{
		fprintf(stderr, "WaitNamedPipe failure: %s\n", pipeName);
		return NULL;
	}

	return pipeName;
}

int qt_rds_module_stop(RDS_MODULE_COMMON* module)
{
	rdsModuleQt* qt = (rdsModuleQt*) module;
	DWORD ret;

	WLog_Print(qt->log, WLOG_DEBUG, "RdsModuleStop");
	TerminateProcess(qt->pi.hProcess,0);

	 // Wait until child process exits.
	WaitForSingleObject(qt->pi.hProcess, INFINITE);

	GetExitCodeProcess(qt->pi.hProcess, &ret);
	WLog_Print(qt->log, WLOG_DEBUG, "terminated process returned %d", ret);

	CloseHandle(qt->pi.hProcess);
	CloseHandle(qt->pi.hThread);

	return 0;
}

int RdsModuleEntry(RDS_MODULE_ENTRY_POINTS* pEntryPoints)
{
	pEntryPoints->Version = 1;
	pEntryPoints->Name = "Qt";

	pEntryPoints->New = qt_rds_module_new;
	pEntryPoints->Free = qt_rds_module_free;

	pEntryPoints->Start = qt_rds_module_start;
	pEntryPoints->Stop = qt_rds_module_stop;

	gStatus = pEntryPoints->status;
	gConfig = pEntryPoints->config;

	return 0;
}

