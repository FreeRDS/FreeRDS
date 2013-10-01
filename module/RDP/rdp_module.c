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

#include <freerds/freerds.h>

#include "rdp_module.h"

struct rds_module_rdp
{
	rdsConnector connector;

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
};
typedef struct rds_module_rdp rdsModuleRdp;

int rdp_rds_module_new(rdsModule* module)
{
	return 0;
}

void rdp_rds_module_free(rdsModule* module)
{

}

int rdp_rds_module_start(rdsModule* module)
{
	BOOL status;
	rdsModuleRdp* rdp;
	rdpSettings* settings;
	rdsConnector* connector;
	char lpCommandLine[256];

	rdp = (rdsModuleRdp*) module;
	connector = (rdsConnector*) module;

	settings = connector->settings;

	freerds_named_pipe_clean(module->SessionId, "RDP");

	ZeroMemory(&(rdp->si), sizeof(STARTUPINFO));
	rdp->si.cb = sizeof(STARTUPINFO);
	ZeroMemory(&(rdp->pi), sizeof(PROCESS_INFORMATION));

	sprintf_s(lpCommandLine, sizeof(lpCommandLine), "%s /tmp/rds.rdp /session-id:%d /size:%dx%d",
			"freerds-rdp", (int) module->SessionId, settings->DesktopWidth, settings->DesktopHeight);

	status = CreateProcessA(NULL, lpCommandLine,
			NULL, NULL, FALSE, 0, NULL, NULL,
			&(rdp->si), &(rdp->pi));

	printf("Process started: %d\n", status);

	module->hClientPipe = freerds_named_pipe_connect(module->SessionId, "RDP", 5 * 1000);

	return 0;
}

int rdp_rds_module_stop(rdsModule* module)
{
	rdsConnector* connector;

	connector = (rdsConnector*) module;

	SetEvent(connector->StopEvent);

	return 0;
}

int RDP_RdsModuleEntry(RDS_MODULE_ENTRY_POINTS* pEntryPoints)
{
	pEntryPoints->Version = 1;
	pEntryPoints->Size = sizeof(RDS_MODULE_ENTRY_POINTS_V1);

	pEntryPoints->ContextSize = sizeof(rdsModuleRdp);
	pEntryPoints->New = rdp_rds_module_new;
	pEntryPoints->Free = rdp_rds_module_free;

	pEntryPoints->Start = rdp_rds_module_start;
	pEntryPoints->Stop = rdp_rds_module_stop;

	return 0;
}

