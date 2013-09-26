/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * X11 Server Module
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

#include <xrdp-ng/xrdp.h>

#include "x11_module.h"

struct rds_module_x11
{
	xrdpModule module;

	STARTUPINFO X11StartupInfo;
	PROCESS_INFORMATION X11ProcessInformation;

	STARTUPINFO WMStartupInfo;
	PROCESS_INFORMATION WMProcessInformation;
};
typedef struct rds_module_x11 rdsModuleX11;

int x11_rds_module_new(rdsModule* module)
{
	return 0;
}

void x11_rds_module_free(rdsModule* module)
{

}

int x11_rds_module_start(rdsModule* module)
{
	BOOL status;
	HANDLE token;
	DWORD SessionId;
	char envstr[256];
	rdsModuleX11* x11;
	struct passwd* pwnam;
	rdpSettings* settings;
	char lpCommandLine[256];

	x11 = (rdsModuleX11*) module;

	token = NULL;
	settings = module->settings;
	SessionId = module->SessionId;

	freerds_named_pipe_clean(SessionId, "X11rdp");

	LogonUserA(settings->Username, settings->Domain, settings->Password,
			LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &token);

	pwnam = getpwnam(settings->Username);

	sprintf_s(envstr, sizeof(envstr), ":%d", (int) SessionId);
	SetEnvironmentVariableA("DISPLAY", envstr);

	SetEnvironmentVariableA("SHELL", pwnam->pw_shell);
	SetEnvironmentVariableA("USER", pwnam->pw_name);
	SetEnvironmentVariableA("HOME", pwnam->pw_dir);

	sprintf_s(envstr, sizeof(envstr), "%d", (int) pwnam->pw_uid);
	SetEnvironmentVariableA("UID", envstr);

	ZeroMemory(&(x11->X11StartupInfo), sizeof(STARTUPINFO));
	x11->X11StartupInfo.cb = sizeof(STARTUPINFO);
	ZeroMemory(&(x11->X11ProcessInformation), sizeof(PROCESS_INFORMATION));

	sprintf_s(lpCommandLine, sizeof(lpCommandLine), "%s :%d -geometry %dx%d -depth %d -uds",
			"X11rdp", (int) SessionId, settings->DesktopWidth, settings->DesktopHeight, 24);

	status = CreateProcessA(NULL, lpCommandLine,
			NULL, NULL, FALSE, 0, NULL, NULL,
			&(x11->X11StartupInfo), &(x11->X11ProcessInformation));

	fprintf(stderr, "Process started: %d\n", status);

	module->hClientPipe = freerds_named_pipe_connect(SessionId, "X11rdp", 5 * 1000);

	ZeroMemory(&(x11->WMStartupInfo), sizeof(STARTUPINFO));
	x11->WMStartupInfo.cb = sizeof(STARTUPINFO);
	ZeroMemory(&(x11->WMProcessInformation), sizeof(PROCESS_INFORMATION));

	status = CreateProcessAsUserA(token,
			NULL, "startwm.sh",
			NULL, NULL, FALSE, 0, NULL, NULL,
			&(x11->WMStartupInfo), &(x11->WMProcessInformation));

	fprintf(stderr, "User process started: %d\n", status);

	return 0;
}

int x11_rds_module_stop(rdsModule* module)
{
	SetEvent(module->StopEvent);

#if 0
	WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

	status = GetExitCodeProcess(ProcessInformation.hProcess, &exitCode);

	CloseHandle(ProcessInformation.hProcess);
	CloseHandle(ProcessInformation.hThread);
#endif

	return 0;
}

int X11_RdsModuleEntry(RDS_MODULE_ENTRY_POINTS* pEntryPoints)
{
	pEntryPoints->Version = 1;
	pEntryPoints->Size = sizeof(RDS_MODULE_ENTRY_POINTS_V1);

	pEntryPoints->ContextSize = sizeof(rdsModuleX11);
	pEntryPoints->New = x11_rds_module_new;
	pEntryPoints->Free = x11_rds_module_free;

	pEntryPoints->Start = x11_rds_module_start;
	pEntryPoints->Stop = x11_rds_module_stop;

	return 0;
}
