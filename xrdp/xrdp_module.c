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

int xrdp_client_capabilities(xrdpModule* mod)
{
	wStream* s;
	int length;
	rdpSettings* settings;
	XRDP_MSG_CAPABILITIES msg;

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	msg.msgFlags = 0;
	msg.type = XRDP_CLIENT_CAPABILITIES;

	settings = mod->settings;

	msg.DesktopWidth = settings->DesktopWidth;
	msg.DesktopHeight= settings->DesktopHeight;
	msg.ColorDepth = settings->ColorDepth;

	length = xrdp_write_capabilities(NULL, &msg);

	xrdp_write_capabilities(s, &msg);

	freerds_named_pipe_write(mod->hClientPipe, Stream_Buffer(s), length);

	return 0;
}

int xrdp_client_connect(xrdpModule* mod)
{
	int length;
	wStream* s;
	RECTANGLE_16 rect;
	XRDP_MSG_OPAQUE_RECT opaqueRect;
	XRDP_MSG_BEGIN_UPDATE beginUpdate;
	XRDP_MSG_END_UPDATE endUpdate;
	XRDP_MSG_REFRESH_RECT refreshRect;

	beginUpdate.type = XRDP_SERVER_BEGIN_UPDATE;
	endUpdate.type = XRDP_SERVER_END_UPDATE;

	opaqueRect.type = XRDP_SERVER_OPAQUE_RECT;
	opaqueRect.nTopRect = 0;
	opaqueRect.nLeftRect = 0;
	opaqueRect.nWidth = mod->settings->DesktopWidth;
	opaqueRect.nHeight = mod->settings->DesktopHeight;
	opaqueRect.color = 0;

	/* clear screen */

	mod->server->BeginUpdate(mod, &beginUpdate);
	mod->server->OpaqueRect(mod, &opaqueRect);
	mod->server->EndUpdate(mod, &endUpdate);

	xrdp_client_capabilities(mod);

	refreshRect.msgFlags = 0;
	refreshRect.type = XRDP_CLIENT_REFRESH_RECT;

	refreshRect.numberOfAreas = 1;
	refreshRect.areasToRefresh = &rect;

	rect.left = 0;
	rect.top = 0;
	rect.right = mod->settings->DesktopWidth - 1;
	rect.bottom = mod->settings->DesktopHeight - 1;

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_refresh_rect(NULL, &refreshRect);

	xrdp_write_refresh_rect(s, &refreshRect);
	freerds_named_pipe_write(mod->hClientPipe, Stream_Buffer(s), length);

	ResumeThread(mod->ServerThread);

	return 0;
}

int xrdp_client_get_event_handles(xrdpModule* mod, HANDLE* events, DWORD* nCount)
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

int xrdp_client_check_event_handles(xrdpModule* mod)
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

xrdpModule* xrdp_module_new(xrdpSession* session)
{
	int error_code;
	int auth_status;
	xrdpModule* mod;
	rdpSettings* settings;
	RDS_MODULE_ENTRY_POINTS EntryPoints;

	ZeroMemory(&EntryPoints, sizeof(EntryPoints));
	EntryPoints.Version = RDS_MODULE_INTERFACE_VERSION;
	EntryPoints.Size = sizeof(EntryPoints);

	X11_RdsModuleEntry(&EntryPoints);

	settings = session->settings;

	auth_status = xrdp_authenticate(settings->Username, settings->Password, &error_code);

	mod = (xrdpModule*) malloc(EntryPoints.ContextSize);
	ZeroMemory(mod, EntryPoints.ContextSize);

	mod->size = EntryPoints.ContextSize;
	mod->session = session;
	mod->settings = session->settings;
	mod->SessionId = 10;

	mod->pEntryPoints = (RDS_MODULE_ENTRY_POINTS*) malloc(sizeof(RDS_MODULE_ENTRY_POINTS));
	CopyMemory(mod->pEntryPoints, &EntryPoints, sizeof(RDS_MODULE_ENTRY_POINTS));

	mod->Connect = xrdp_client_connect;
	mod->GetEventHandles = xrdp_client_get_event_handles;
	mod->CheckEventHandles = xrdp_client_check_event_handles;

	mod->client = freerds_client_outbound_interface_new();
	mod->server = freerds_server_outbound_interface_new();

	mod->SendStream = Stream_New(NULL, 8192);
	mod->ReceiveStream = Stream_New(NULL, 8192);

	mod->TotalLength = 0;
	mod->TotalCount = 0;

	mod->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	mod->pEntryPoints->New((rdsModule*) mod);

	mod->ServerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) xrdp_client_thread,
			(void*) mod, CREATE_SUSPENDED, NULL);

	freerds_client_inbound_module_init(mod);

	mod->pEntryPoints->Start((rdsModule*) mod);

	if (mod->Connect(mod) != 0)
		return NULL;

	return mod;
}

void xrdp_module_free(xrdpModule* mod)
{
	SetEvent(mod->StopEvent);

	WaitForSingleObject(mod->ServerThread, INFINITE);
	CloseHandle(mod->ServerThread);

	Stream_Free(mod->SendStream, TRUE);
	Stream_Free(mod->ReceiveStream, TRUE);

	CloseHandle(mod->StopEvent);
	CloseHandle(mod->hClientPipe);

	free(mod);
}
