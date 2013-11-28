/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * CEF Server Module
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
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/pipe.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <freerds/backend.h>

#include "cef_module.h"

RDS_MODULE_CONFIG_CALLBACKS gConfig;
RDS_MODULE_STATUS_CALLBACKS gStatus;


struct rds_module_cef
{
	RDS_MODULE_COMMON commonModule;

	wLog* log;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
};
typedef struct rds_module_cef rdsModuleCef;

RDS_MODULE_COMMON * cef_rds_module_new(void )
{
	rdsModuleCef* cef = (rdsModuleCef*) malloc(sizeof(rdsModuleCef));

	WLog_Init();

	cef->log = WLog_Get("com.freerds.module.cef");
	WLog_OpenAppender(cef->log);

	WLog_SetLogLevel(cef->log, WLOG_DEBUG);

	WLog_Print(cef->log, WLOG_DEBUG, "RdsModuleNew");

	return (RDS_MODULE_COMMON *)cef;
}

void cef_rds_module_free(RDS_MODULE_COMMON * module)
{

	rdsModuleCef* cef = (rdsModuleCef *)module;
	WLog_Print(cef->log, WLOG_DEBUG, "RdsModuleFree");
	WLog_Uninit();
	free(module);
}

char * cef_rds_module_start(RDS_MODULE_COMMON * module)
{
	BOOL status;
	rdsModuleCef* cef = (rdsModuleCef *)module;
	char lpCommandLine[256];
	const char* endpoint = "CEF";
	DWORD SessionId = cef->commonModule.sessionId;
	HANDLE hClientPipe;
	char* pipeName;

	WLog_Print(cef->log, WLOG_DEBUG, "RdsModuleStart: SessionId: %d Endpoint: %s",
			(int) SessionId, endpoint);

	freerds_named_pipe_clean_endpoint(SessionId, endpoint);

	ZeroMemory(&(cef->si), sizeof(STARTUPINFO));
	cef->si.cb = sizeof(STARTUPINFO);
	ZeroMemory(&(cef->pi), sizeof(PROCESS_INFORMATION));

	sprintf_s(lpCommandLine, sizeof(lpCommandLine), "%s /session-id:%d /width:%d /height:%d",
			"freerds-cef", (int) SessionId, 1024, 768);

	WLog_Print(cef->log, WLOG_DEBUG, "Starting process with command line: %s", lpCommandLine);

	status = CreateProcessA(NULL, lpCommandLine,
			NULL, NULL, FALSE, 0, NULL, NULL,
			&(cef->si), &(cef->pi));

	WLog_Print(cef->log, WLOG_DEBUG, "Process created with status: %d", status);

	pipeName = (char *)malloc(256);
	sprintf_s(pipeName, 256, "\\\\.\\pipe\\FreeRDS_%d_%s", (int) SessionId, "CEF");

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

int cef_rds_module_stop(RDS_MODULE_COMMON * module)
{
	rdsModuleCef* cef = (rdsModuleCef *)module;

	WLog_Print(cef->log, WLOG_DEBUG, "RdsModuleStop");
	return 0;
}

int RdsModuleEntry(RDS_MODULE_ENTRY_POINTS* pEntryPoints)
{
	pEntryPoints->Version = 1;
	pEntryPoints->Name = "CEF";

	pEntryPoints->New = cef_rds_module_new;
	pEntryPoints->Free = cef_rds_module_free;

	pEntryPoints->Start = cef_rds_module_start;
	pEntryPoints->Stop = cef_rds_module_stop;

	gStatus = pEntryPoints->status;
	gConfig = pEntryPoints->config;


	return 0;
}

