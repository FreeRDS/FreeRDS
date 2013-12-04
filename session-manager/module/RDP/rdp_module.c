/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * RDP Server Module
 *
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright 2013 Thincast Technologies GmbH
 * Copyright 2013 DI (FH) Martin Haimberger <martin.haimberger@thincast.com>
 *
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
#include <winpr/environment.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <freerds/module.h>
#include <freerds/backend.h>

#include "rdp_module.h"

RDS_MODULE_CONFIG_CALLBACKS gConfig;
RDS_MODULE_STATUS_CALLBACKS gStatus;

struct rds_module_rdp
{
	RDS_MODULE_COMMON commonModule;

	wLog* log;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	DWORD SessionId;
	HANDLE hClientPipe;

};
typedef struct rds_module_rdp rdsModuleRdp;

RDS_MODULE_COMMON* rdp_rds_module_new(void)
{
	rdsModuleRdp* rdp;

	WLog_Init();

	rdp = (rdsModuleRdp*) malloc(sizeof(rdsModuleRdp));

	rdp->log = WLog_Get("com.freerds.module.rdp");
	WLog_OpenAppender(rdp->log);

	WLog_SetLogLevel(rdp->log, WLOG_DEBUG);

	WLog_Print(rdp->log, WLOG_DEBUG, "RdsModuleNew");

	return (RDS_MODULE_COMMON*) rdp;
}

void rdp_rds_module_free(RDS_MODULE_COMMON* module)
{
	rdsModuleRdp* rdp;

	rdp = (rdsModuleRdp*) module;

	WLog_Print(rdp->log, WLOG_DEBUG, "RdsModuleFree");

	WLog_Uninit();
	free(rdp);
}

void initResolutions(rdsModuleRdp * rdp,  long * xres, long * yres, long * colordepth) {
	char tempstr[256];

	long maxXRes, maxYRes, minXRes, minYRes = 0;
	long connectionXRes, connectionYRes, connectionColorDepth = 0;

	if (!gConfig.getPropertyNumber(rdp->commonModule.sessionId, "module.rdp.maxXRes", &maxXRes)) {
		WLog_Print(rdp->log, WLOG_ERROR, "Setting: module.rdp.maxXRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n");
	}
	if (!gConfig.getPropertyNumber(rdp->commonModule.sessionId, "module.rdp.maxYRes", &maxYRes)) {
		WLog_Print(rdp->log, WLOG_ERROR, "Setting: module.rdp.maxYRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n");
	}
	if (!gConfig.getPropertyNumber(rdp->commonModule.sessionId, "module.rdp.minXRes", &minXRes)) {
		WLog_Print(rdp->log, WLOG_ERROR, "Setting: module.rdp.minXRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n");
	}
	if (!gConfig.getPropertyNumber(rdp->commonModule.sessionId, "module.rdp.minYRes", &minYRes)){
		WLog_Print(rdp->log, WLOG_ERROR, "Setting: module.rdp.minYRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n");
	}

	if ((maxXRes != 0) && (maxYRes != 0) && (minXRes != 0) && (minYRes != 0)) {
		sprintf_s(tempstr, sizeof(tempstr), "%dx%d", (unsigned int) maxXRes,(unsigned int) maxYRes );
		SetEnvironmentVariableEBA(&rdp->commonModule.envBlock, "FREERDS_SMAX", tempstr);

		sprintf_s(tempstr, sizeof(tempstr), "%dx%d", (unsigned int) minXRes,(unsigned int) minYRes );
		SetEnvironmentVariableEBA(&rdp->commonModule.envBlock, "FREERDS_SMIN", tempstr);
	}

	gConfig.getPropertyNumber(rdp->commonModule.sessionId, "connection.xres", &connectionXRes);
	gConfig.getPropertyNumber(rdp->commonModule.sessionId, "connection.yres", &connectionYRes);
	gConfig.getPropertyNumber(rdp->commonModule.sessionId, "connection.colordepth", &connectionColorDepth);

	if ((connectionXRes == 0) || (connectionYRes == 0)) {
		WLog_Print(rdp->log, WLOG_ERROR, "got no XRes or YRes from client, using config values");

		if (!gConfig.getPropertyNumber(rdp->commonModule.sessionId, "module.rdp.xres", xres))
			*xres = 1024;

		if (!gConfig.getPropertyNumber(rdp->commonModule.sessionId, "module.rdp.yres", yres))
			*yres = 768;

		if (!gConfig.getPropertyNumber(rdp->commonModule.sessionId, "module.rdp.colordepth", colordepth))
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


char* rdp_rds_module_start(RDS_MODULE_COMMON* module)
{
	BOOL status;
	rdsModuleRdp* rdp;
	char lpCommandLine[256];
	const char* endpoint = "RDP";
	long xres, yres,colordepth;
	char* pipeName = (char*) malloc(256);

	rdp = (rdsModuleRdp*) module;

	rdp->SessionId = rdp->commonModule.sessionId;

	WLog_Print(rdp->log, WLOG_DEBUG, "RdsModuleStart: SessionId: %d Endpoint: %s",
			(int) rdp->commonModule.sessionId, endpoint);

	freerds_named_pipe_get_endpoint_name((int) rdp->commonModule.sessionId, endpoint, pipeName, 256);
	freerds_named_pipe_clean(pipeName);

	ZeroMemory(&(rdp->si), sizeof(STARTUPINFO));
	rdp->si.cb = sizeof(STARTUPINFO);
	ZeroMemory(&(rdp->pi), sizeof(PROCESS_INFORMATION));

	initResolutions(rdp,&xres,&yres,&colordepth);

	sprintf_s(lpCommandLine, sizeof(lpCommandLine), "%s /tmp/rds.rdp /size:%dx%d",
			"freerds-rdp", (int) xres, (int) yres);

	WLog_Print(rdp->log, WLOG_DEBUG, "Starting process with command line: %s", lpCommandLine);

	status = CreateProcessA(NULL, lpCommandLine,
			NULL, NULL, FALSE, 0, rdp->commonModule.envBlock, NULL,
			&(rdp->si), &(rdp->pi));

	WLog_Print(rdp->log, WLOG_DEBUG, "Process created with status: %d", status);

	if (!WaitNamedPipeA(pipeName, 5 * 1000))
	{
		fprintf(stderr, "WaitNamedPipe failure: %s\n", pipeName);
		return NULL;
	}

	return pipeName;
}

int rdp_rds_module_stop(RDS_MODULE_COMMON* module)
{
	rdsModuleRdp* rdp;

	rdp = (rdsModuleRdp*) module;

	WLog_Print(rdp->log, WLOG_DEBUG, "RdsModuleStop");
	return 0;
}

int RdsModuleEntry(RDS_MODULE_ENTRY_POINTS* pEntryPoints)
{
	pEntryPoints->Version = 1;
	pEntryPoints->Name = "RDP";

	pEntryPoints->New = rdp_rds_module_new;
	pEntryPoints->Free = rdp_rds_module_free;

	pEntryPoints->Start = rdp_rds_module_start;
	pEntryPoints->Stop = rdp_rds_module_stop;

	gStatus = pEntryPoints->status;
	gConfig = pEntryPoints->config;

	return 0;
}
