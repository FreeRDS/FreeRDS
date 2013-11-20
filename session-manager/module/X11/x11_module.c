/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * X11 Server Module
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

#include <pwd.h>
#include <grp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/stat.h>

#include <sys/ioctl.h>
#include <sys/socket.h>

#include <winpr/crt.h>
#include <winpr/pipe.h>
#include <winpr/path.h>
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/stream.h>
#include <winpr/sspicli.h>
#include <winpr/environment.h>
#include <freerds/freerds.h>

#include "x11_module.h"

pgetPropertyBool gGetPropertyBool;
pgetPropertyNumber gGetPropertyNumber;
pgetPropertyString gGetPropertyString;


struct rds_module_x11
{
	RDS_MODULE_COMMON commonModule;

	STARTUPINFO X11StartupInfo;
	PROCESS_INFORMATION X11ProcessInformation;

	STARTUPINFO WMStartupInfo;
	PROCESS_INFORMATION WMProcessInformation;

};
typedef struct rds_module_x11 rdsModuleX11;

void x11_rds_module_reset_process_informations(rdsModuleX11* module)
{
	ZeroMemory(&(module->X11StartupInfo), sizeof(STARTUPINFO));
	module->X11StartupInfo.cb = sizeof(STARTUPINFO);
	ZeroMemory(&(module->X11ProcessInformation), sizeof(PROCESS_INFORMATION));
	ZeroMemory(&(module->WMStartupInfo), sizeof(STARTUPINFO));
	module->WMStartupInfo.cb = sizeof(STARTUPINFO);
	ZeroMemory(&(module->WMProcessInformation), sizeof(PROCESS_INFORMATION));
}

RDS_MODULE_COMMON* x11_rds_module_new(void)
{
	rdsModuleX11* module = (rdsModuleX11*) malloc(sizeof(rdsModuleX11));
	ZeroMemory(module, sizeof(rdsModuleX11));

	x11_rds_module_reset_process_informations(module);

	module->commonModule.sessionId = 0;
	module->commonModule.userToken = NULL;
	module->commonModule.authToken = NULL;

	return (RDS_MODULE_COMMON*) module;
}

void x11_rds_module_free(RDS_MODULE_COMMON* module)
{
	rdsModuleX11* moduleCon = (rdsModuleX11*) module;

	if (moduleCon->commonModule.authToken)
		free(moduleCon->commonModule.authToken);

	/*if (moduleCon->commonModule.userToken)
		CloseHandle(moduleCon->commonModule.userToken);*/

	free(module);
}

char* x11_rds_module_start(RDS_MODULE_COMMON * module)
{
	BOOL status;
	DWORD SessionId;
	DWORD displayNum;
	char envstr[256];
	rdsModuleX11* x11;
	struct passwd* pwnam;
	char lpCommandLine[256];

	char* filename;
	char* pipeName;
	long xres, yres, colordepth;

	x11 = (rdsModuleX11*) module;

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

	pwnam = getpwnam(x11->commonModule.userName);

	sprintf_s(envstr, sizeof(envstr), ":%d", (int) (displayNum));
	SetEnvironmentVariableEBA(&x11->commonModule.envBlock, "DISPLAY", envstr);

	if (!gGetPropertyNumber(x11->commonModule.sessionId, "module.x11.xres", &xres))
		xres = 1024;

	if (!gGetPropertyNumber(x11->commonModule.sessionId, "module.x11.yres", &yres))
		yres = 768;

	if (!gGetPropertyNumber(x11->commonModule.sessionId, "module.x11.colordepth", &colordepth))
		colordepth = 24;

	x11_rds_module_reset_process_informations(x11);

	sprintf_s(lpCommandLine, sizeof(lpCommandLine), "%s :%d -geometry %dx%d -depth %d -uds -terminate",
			"X11rdp", (int) (displayNum), (int) xres, (int) yres, (int) colordepth);

	status = CreateProcessA(NULL, lpCommandLine,
			NULL, NULL, FALSE, 0, x11->commonModule.envBlock, NULL,
			&(x11->X11StartupInfo), &(x11->X11ProcessInformation));

	fprintf(stderr, "Process started: %d\n", status);

	if (!WaitNamedPipeA(pipeName, 5 * 1000))
	{
		fprintf(stderr, "WaitNamedPipe failure: %s\n", pipeName);
		return NULL;
	}
#if 0
	hClientPipe = CreateFileA(pipeName,
			GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if ((!hClientPipe) || (hClientPipe == INVALID_HANDLE_VALUE))
	{
		fprintf(stderr, "Failed to create named pipe %s\n", pipeName);
		return NULL;
	}
	CloseHandle(hClientPipe);
#endif

	status = CreateProcessAsUserA(x11->commonModule.userToken,
			NULL, "startwm.sh",
			NULL, NULL, FALSE, 0, x11->commonModule.envBlock, NULL,
			&(x11->WMStartupInfo), &(x11->WMProcessInformation));

	fprintf(stderr, "User process started: %d\n", status);

	return pipeName;
}

int x11_rds_module_stop(RDS_MODULE_COMMON * module)
{
	/*rdsConnector* connector;

	connector = (rdsConnector*) module;

	SetEvent(connector->StopEvent);*/

#if 0
	WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

	status = GetExitCodeProcess(ProcessInformation.hProcess, &exitCode);

	CloseHandle(ProcessInformation.hProcess);
	CloseHandle(ProcessInformation.hThread);
#endif

	return 0;
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

	gGetPropertyBool = pEntryPoints->getPropertyBool;
	gGetPropertyNumber = pEntryPoints->getPropertyNumber;
	gGetPropertyString = pEntryPoints->getPropertyString;

	gGetPropertyBool = pEntryPoints->getPropertyBool;
	gGetPropertyNumber = pEntryPoints->getPropertyNumber;
	gGetPropertyString = pEntryPoints->getPropertyString;

	return 0;
}
