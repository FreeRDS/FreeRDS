/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2013
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
 *
 * module manager
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xrdp.h"

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

#include <freerdp/freerdp.h>

#include "../module/X11/x11_module.h"

int xrdp_client_get_event_handles(rdsModule* mod, HANDLE* events, DWORD* nCount)
{
	if (mod)
	{
		if (mod->ServerQueue)
		{
			events[*nCount] = MessageQueue_Event(mod->ServerQueue);
			(*nCount)++;
		}
	}

	return 0;
}

int xrdp_client_check_event_handles(rdsModule* mod)
{
	int status = 0;

	if (!mod)
		return 0;

	while (WaitForSingleObject(MessageQueue_Event(mod->ServerQueue), 0) == WAIT_OBJECT_0)
	{
		status = xrdp_message_server_queue_process_pending_messages(mod);
	}

	return status;
}

rdsModule* xrdp_module_new(xrdpSession* session)
{
	int error_code;
	int auth_status;
	rdsModule* mod;
	rdpSettings* settings;
	RDS_MODULE_ENTRY_POINTS EntryPoints;

	ZeroMemory(&EntryPoints, sizeof(EntryPoints));
	EntryPoints.Version = RDS_MODULE_INTERFACE_VERSION;
	EntryPoints.Size = sizeof(EntryPoints);

	X11_RdsModuleEntry(&EntryPoints);

	settings = session->settings;

	auth_status = xrdp_authenticate(settings->Username, settings->Password, &error_code);

	mod = (rdsModule*) malloc(EntryPoints.ContextSize);
	ZeroMemory(mod, EntryPoints.ContextSize);

	mod->Size = EntryPoints.ContextSize;
	mod->session = session;
	mod->settings = session->settings;
	mod->SessionId = 10;

	mod->pEntryPoints = (RDS_MODULE_ENTRY_POINTS*) malloc(sizeof(RDS_MODULE_ENTRY_POINTS));
	CopyMemory(mod->pEntryPoints, &EntryPoints, sizeof(RDS_MODULE_ENTRY_POINTS));

	mod->GetEventHandles = xrdp_client_get_event_handles;
	mod->CheckEventHandles = xrdp_client_check_event_handles;

	mod->client = freerds_client_outbound_interface_new();
	mod->server = freerds_server_outbound_interface_new();

	mod->OutboundStream = Stream_New(NULL, 8192);
	mod->InboundStream = Stream_New(NULL, 8192);

	mod->InboundTotalLength = 0;
	mod->InboundTotalCount = 0;

	mod->OutboundTotalLength = 0;
	mod->OutboundTotalCount = 0;

	mod->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	mod->pEntryPoints->New((rdsModule*) mod);

	mod->ServerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) xrdp_client_thread,
			(void*) mod, CREATE_SUSPENDED, NULL);

	freerds_client_inbound_module_init(mod);

	mod->pEntryPoints->Start(mod);

	ResumeThread(mod->ServerThread);

	return mod;
}

void xrdp_module_free(rdsModule* mod)
{
	SetEvent(mod->StopEvent);

	WaitForSingleObject(mod->ServerThread, INFINITE);
	CloseHandle(mod->ServerThread);

	Stream_Free(mod->OutboundStream, TRUE);
	Stream_Free(mod->InboundStream, TRUE);

	CloseHandle(mod->StopEvent);
	CloseHandle(mod->hClientPipe);

	mod->pEntryPoints->Free(mod);

	free(mod);
}
