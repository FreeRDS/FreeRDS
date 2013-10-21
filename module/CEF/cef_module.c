/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * CEF Server Module
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <freerds/freerds.h>

#include "cef_module.h"

struct rds_module_cef
{
	rdsConnector connector;

	wLog* log;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
};
typedef struct rds_module_cef rdsModuleCef;

int cef_rds_module_new(rdsModule* module)
{
	rdsModuleCef* cef;

	WLog_Init();

	cef = (rdsModuleCef*) module;

	cef->log = WLog_Get("com.freerds.module.cef.connector");
	WLog_OpenAppender(cef->log);

	WLog_SetLogLevel(cef->log, WLOG_DEBUG);

	WLog_Print(cef->log, WLOG_DEBUG, "RdsModuleNew");

	return 0;
}

void cef_rds_module_free(rdsModule* module)
{
	rdsModuleCef* cef;

	cef = (rdsModuleCef*) module;

	WLog_Print(cef->log, WLOG_DEBUG, "RdsModuleFree");

	WLog_Uninit();
}

int cef_rds_module_start(rdsModule* module)
{
	BOOL status;
	rdsModuleCef* cef;
	rdpSettings* settings;
	rdsConnector* connector;
	char lpCommandLine[256];
	const char* endpoint = "CEF";

	cef = (rdsModuleCef*) module;
	connector = (rdsConnector*) module;

	WLog_Print(cef->log, WLOG_DEBUG, "RdsModuleStart: SessionId: %d Endpoint: %s",
			(int) module->SessionId, endpoint);

	settings = connector->settings;

	freerds_named_pipe_clean(module->SessionId, endpoint);

	ZeroMemory(&(cef->si), sizeof(STARTUPINFO));
	cef->si.cb = sizeof(STARTUPINFO);
	ZeroMemory(&(cef->pi), sizeof(PROCESS_INFORMATION));

	sprintf_s(lpCommandLine, sizeof(lpCommandLine), "%s /session-id:%d /width:%d /height:%d",
			"freerds-cef", (int) module->SessionId, settings->DesktopWidth, settings->DesktopHeight);

	WLog_Print(cef->log, WLOG_DEBUG, "Starting process with command line: %s", lpCommandLine);

	status = CreateProcessA(NULL, lpCommandLine,
			NULL, NULL, FALSE, 0, NULL, NULL,
			&(cef->si), &(cef->pi));

	WLog_Print(cef->log, WLOG_DEBUG, "Process created with status: %d", status);

	module->hClientPipe = freerds_named_pipe_connect(module->SessionId, "CEF", 5 * 1000);

	if (!module->hClientPipe)
	{
		WLog_Print(cef->log, WLOG_ERROR, "Failed to connect to service");
		return -1;
	}

	return 0;
}

int cef_rds_module_stop(rdsModule* module)
{
	rdsModuleCef* cef;
	rdsConnector* connector;

	cef = (rdsModuleCef*) module;
	connector = (rdsConnector*) module;

	WLog_Print(cef->log, WLOG_DEBUG, "RdsModuleStop");

	SetEvent(connector->StopEvent);

	return 0;
}

int RdsModuleEntry(RDS_MODULE_ENTRY_POINTS* pEntryPoints)
{
	pEntryPoints->Version = 1;
	pEntryPoints->Size = sizeof(RDS_MODULE_ENTRY_POINTS_V1);
	pEntryPoints->Name = "CEF";

	pEntryPoints->ContextSize = sizeof(rdsModuleCef);
	pEntryPoints->New = cef_rds_module_new;
	pEntryPoints->Free = cef_rds_module_free;

	pEntryPoints->Start = cef_rds_module_start;
	pEntryPoints->Stop = cef_rds_module_stop;

	return 0;
}

