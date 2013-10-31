/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * RDP Server Module
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <freerds/freerds.h>

#include "rdp_module.h"

struct rds_module_rdp
{
	rdsModuleConnector connector;

	wLog* log;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	DWORD SessionId;
	HANDLE hClientPipe;

};
typedef struct rds_module_rdp rdsModuleRdp;

int rdp_rds_module_new(void * module)
{
	rdsModuleRdp* rdp;

	WLog_Init();

	rdp = (rdsModuleRdp*) module;

	rdp->log = WLog_Get("com.freerds.module.rdp.connector");
	WLog_OpenAppender(rdp->log);

	WLog_SetLogLevel(rdp->log, WLOG_DEBUG);

	WLog_Print(rdp->log, WLOG_DEBUG, "RdsModuleNew");

	return 0;
}

void rdp_rds_module_free(void * module)
{
	rdsModuleRdp* rdp;

	rdp = (rdsModuleRdp*) module;

	WLog_Print(rdp->log, WLOG_DEBUG, "RdsModuleFree");

	WLog_Uninit();
}

int rdp_rds_module_start(void * module)
{
	BOOL status;
	rdsModuleRdp* rdp;
	rdpSettings* settings;
	rdsModuleConnector* connector;

	char lpCommandLine[256];
	const char* endpoint = "RDP";

	rdp = (rdsModuleRdp*) module;

	WLog_Print(rdp->log, WLOG_DEBUG, "RdsModuleStart: SessionId: %d Endpoint: %s",
			(int) rdp->SessionId, endpoint);

	settings = connector->settings;

	freerds_named_pipe_clean_endpoint(rdp->SessionId, endpoint);

	ZeroMemory(&(rdp->si), sizeof(STARTUPINFO));
	rdp->si.cb = sizeof(STARTUPINFO);
	ZeroMemory(&(rdp->pi), sizeof(PROCESS_INFORMATION));

	sprintf_s(lpCommandLine, sizeof(lpCommandLine), "%s /tmp/rds.rdp /session-id:%d /size:%dx%d",
			"freerds-rdp", (int) rdp->SessionId, settings->DesktopWidth, settings->DesktopHeight);

	WLog_Print(rdp->log, WLOG_DEBUG, "Starting process with command line: %s", lpCommandLine);

	status = CreateProcessA(NULL, lpCommandLine,
			NULL, NULL, FALSE, 0, NULL, NULL,
			&(rdp->si), &(rdp->pi));

	WLog_Print(rdp->log, WLOG_DEBUG, "Process created with status: %d", status);

	rdp->hClientPipe = freerds_named_pipe_connect_endpoint(rdp->SessionId, "RDP", 5 * 1000);

	if (!rdp->hClientPipe)
	{
		WLog_Print(rdp->log, WLOG_ERROR, "Failed to connect to service");
		return -1;
	}

	return 0;
}

int rdp_rds_module_stop(void * module)
{
	rdsModuleRdp* rdp;
	rdsModuleConnector* connector;



	rdp = (rdsModuleRdp*) module;
	connector = (rdsModuleConnector*) module;

	WLog_Print(rdp->log, WLOG_DEBUG, "RdsModuleStop");

	SetEvent(connector->StopEvent);

	return 0;
}

int RdsModuleEntry(RDS_MODULE_ENTRY_POINTS* pEntryPoints)
{
	pEntryPoints->Version = 1;
	pEntryPoints->Name = RDP_MODULE_NAME;

	pEntryPoints->New = rdp_rds_module_new;
	pEntryPoints->Free = rdp_rds_module_free;

	pEntryPoints->Start = rdp_rds_module_start;
	pEntryPoints->Stop = rdp_rds_module_stop;

	return 0;
}

