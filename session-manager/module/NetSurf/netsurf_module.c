/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * NetSurf Server Module
 *
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/pipe.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <freerds/freerds.h>

#include "netsurf_module.h"

struct rds_module_netsurf
{
	RDS_MODULE_COMMON commonModule;

	wLog* log;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
};
typedef struct rds_module_netsurf rdsModuleNetSurf;

RDS_MODULE_COMMON* netsurf_rds_module_new(void)
{
	rdsModuleNetSurf* ns = (rdsModuleNetSurf*) malloc(sizeof(rdsModuleNetSurf));

	WLog_Init();

	ns->log = WLog_Get("com.freerds.module.cef");
	WLog_OpenAppender(ns->log);

	WLog_SetLogLevel(ns->log, WLOG_DEBUG);

	WLog_Print(ns->log, WLOG_DEBUG, "RdsModuleNew");

	return (RDS_MODULE_COMMON*) ns;
}

void netsurf_rds_module_free(RDS_MODULE_COMMON * module)
{
	rdsModuleNetSurf* ns = (rdsModuleNetSurf*) module;
	WLog_Print(ns->log, WLOG_DEBUG, "RdsModuleFree");
	WLog_Uninit();
	free(module);
}

char* netsurf_rds_module_start(RDS_MODULE_COMMON* module)
{
	BOOL status;
	char* pipeName;
	HANDLE hClientPipe;
	char lpCommandLine[256];
	const char* endpoint = "NetSurf";
	rdsModuleNetSurf* ns = (rdsModuleNetSurf*) module;
	DWORD SessionId = ns->commonModule.sessionId;

	WLog_Print(ns->log, WLOG_DEBUG, "RdsModuleStart: SessionId: %d Endpoint: %s",
			(int) SessionId, endpoint);

	freerds_named_pipe_clean_endpoint(SessionId, endpoint);

	ZeroMemory(&(ns->si), sizeof(STARTUPINFO));
	ns->si.cb = sizeof(STARTUPINFO);
	ZeroMemory(&(ns->pi), sizeof(PROCESS_INFORMATION));

	sprintf_s(lpCommandLine, sizeof(lpCommandLine), "%s -f freerds -b 32",
			"netsurf");

	WLog_Print(ns->log, WLOG_DEBUG, "Starting process with command line: %s", lpCommandLine);

	status = CreateProcessA(NULL, lpCommandLine,
			NULL, NULL, FALSE, 0, NULL, NULL,
			&(ns->si), &(ns->pi));

	WLog_Print(ns->log, WLOG_DEBUG, "Process created with status: %d", status);

	pipeName = (char *)malloc(256);
	sprintf_s(pipeName, 256, "\\\\.\\pipe\\FreeRDS_%d_%s", (int) SessionId, "NetSurf");

	hClientPipe = CreateFileA(pipeName,
			GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if ((!hClientPipe) || (hClientPipe == INVALID_HANDLE_VALUE))
	{
		fprintf(stderr, "Failed to create named pipe %s\n", pipeName);
		return NULL;
	}

	CloseHandle(hClientPipe);

	return pipeName;
}

int netsurf_rds_module_stop(RDS_MODULE_COMMON* module)
{
	rdsModuleNetSurf* ns = (rdsModuleNetSurf*) module;

	WLog_Print(ns->log, WLOG_DEBUG, "RdsModuleStop");

	return 0;
}

int RdsModuleEntry(RDS_MODULE_ENTRY_POINTS* pEntryPoints)
{
	pEntryPoints->Version = 1;
	pEntryPoints->Name = "NetSurf";

	pEntryPoints->New = netsurf_rds_module_new;
	pEntryPoints->Free = netsurf_rds_module_free;

	pEntryPoints->Start = netsurf_rds_module_start;
	pEntryPoints->Stop = netsurf_rds_module_stop;

	return 0;
}

