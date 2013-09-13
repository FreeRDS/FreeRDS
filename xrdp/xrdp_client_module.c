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
 */

#include "xrdp.h"

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
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/stream.h>

#include <freerdp/freerdp.h>

int xrdp_module_recv(xrdpModule* mod, BYTE* data, int length)
{
	BOOL fSuccess = FALSE;
	DWORD lpNumberOfBytesRead = 0;

	fSuccess = ReadFile(mod->hClientPipe, data, length, &lpNumberOfBytesRead, NULL);

	if (!fSuccess || (lpNumberOfBytesRead == 0))
	{
		return -1;
	}

	return lpNumberOfBytesRead;
}

int xrdp_module_send(xrdpModule* mod, BYTE* data, int length)
{
	BOOL fSuccess = FALSE;
	DWORD lpNumberOfBytesWritten = 0;

	if (!mod->hClientPipe)
		return -1;

	while (length > 0)
	{
		fSuccess = WriteFile(mod->hClientPipe, data, length, &lpNumberOfBytesWritten, NULL);

		if (!fSuccess || (lpNumberOfBytesWritten == 0))
			return -1;

		length -= lpNumberOfBytesWritten;
		data += lpNumberOfBytesWritten;
	}

	return 0;
}

static int lib_send_capabilities(xrdpModule* mod)
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

	xrdp_module_send(mod, Stream_Buffer(s), length);

	return 0;
}

int xrdp_client_start(xrdpModule* mod, int w, int h, int bpp)
{
	mod->width = w;
	mod->height = h;
	mod->bpp = bpp;
	return 0;
}

int xrdp_client_connect(xrdpModule* mod)
{
	int length;
	wStream* s;
	RECTANGLE_16 rect;
	char pipeName[256];
	XRDP_MSG_OPAQUE_RECT opaqueRect;
	XRDP_MSG_BEGIN_UPDATE beginUpdate;
	XRDP_MSG_END_UPDATE endUpdate;
	XRDP_MSG_REFRESH_RECT refreshRect;

	beginUpdate.type = XRDP_SERVER_BEGIN_UPDATE;
	endUpdate.type = XRDP_SERVER_END_UPDATE;

	opaqueRect.type = XRDP_SERVER_OPAQUE_RECT;
	opaqueRect.nTopRect = 0;
	opaqueRect.nLeftRect = 0;
	opaqueRect.nWidth = mod->width;
	opaqueRect.nHeight = mod->height;
	opaqueRect.color = 0;

	/* clear screen */

	mod->server->BeginUpdate(mod, &beginUpdate);
	mod->server->OpaqueRect(mod, &opaqueRect);
	mod->server->EndUpdate(mod, &endUpdate);

	sprintf_s(pipeName, sizeof(pipeName), "\\\\.\\pipe\\FreeRDS_%d_%s", mod->SessionId, "X11rdp");

	mod->hClientPipe = CreateFileA(pipeName,
			GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if ((!mod->hClientPipe) || (mod->hClientPipe == INVALID_HANDLE_VALUE))
	{
		printf("Failed to create named pipe %s\n", pipeName);
		return 1;
	}

	lib_send_capabilities(mod);

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
	xrdp_module_send(mod, Stream_Buffer(s), length);

	ResumeThread(mod->ServerThread);

	return 0;
}

int xrdp_client_synchronize_keyboard_event(xrdpModule* mod, DWORD flags)
{
	int length;
	int status;
	wStream* s;
	XRDP_MSG_SYNCHRONIZE_KEYBOARD_EVENT msg;

	msg.msgFlags = 0;
	msg.type = XRDP_CLIENT_SYNCHRONIZE_KEYBOARD_EVENT;

	msg.flags = flags;

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_synchronize_keyboard_event(NULL, &msg);
	xrdp_write_synchronize_keyboard_event(s, &msg);

	status = xrdp_module_send(mod, Stream_Buffer(s), length);

	return status;
}

int xrdp_client_scancode_keyboard_event(xrdpModule* mod, DWORD flags, DWORD code, DWORD keyboardType)
{
	int length;
	int status;
	wStream* s;
	XRDP_MSG_SCANCODE_KEYBOARD_EVENT msg;

	msg.msgFlags = 0;
	msg.type = XRDP_CLIENT_SCANCODE_KEYBOARD_EVENT;

	msg.flags = flags;
	msg.code = code;
	msg.keyboardType = keyboardType;

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_scancode_keyboard_event(NULL, &msg);
	xrdp_write_scancode_keyboard_event(s, &msg);

	status = xrdp_module_send(mod, Stream_Buffer(s), length);

	return status;
}

int xrdp_client_virtual_keyboard_event(xrdpModule* mod, DWORD flags, DWORD code)
{
	int length;
	int status;
	wStream* s;
	XRDP_MSG_VIRTUAL_KEYBOARD_EVENT msg;

	msg.msgFlags = 0;
	msg.type = XRDP_CLIENT_VIRTUAL_KEYBOARD_EVENT;

	msg.flags = flags;
	msg.code = code;

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_virtual_keyboard_event(NULL, &msg);
	xrdp_write_virtual_keyboard_event(s, &msg);

	status = xrdp_module_send(mod, Stream_Buffer(s), length);

	return status;
}

int xrdp_client_unicode_keyboard_event(xrdpModule* mod, DWORD flags, DWORD code)
{
	int length;
	int status;
	wStream* s;
	XRDP_MSG_UNICODE_KEYBOARD_EVENT msg;

	msg.msgFlags = 0;
	msg.type = XRDP_CLIENT_UNICODE_KEYBOARD_EVENT;

	msg.flags = flags;
	msg.code = code;

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_unicode_keyboard_event(NULL, &msg);
	xrdp_write_unicode_keyboard_event(s, &msg);

	status = xrdp_module_send(mod, Stream_Buffer(s), length);

	return status;
}

int xrdp_client_mouse_event(xrdpModule* mod, DWORD flags, DWORD x, DWORD y)
{
	int length;
	int status;
	wStream* s;
	XRDP_MSG_MOUSE_EVENT msg;

	msg.msgFlags = 0;
	msg.type = XRDP_CLIENT_MOUSE_EVENT;

	msg.flags = flags;
	msg.x = x;
	msg.y = y;

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_mouse_event(NULL, &msg);
	xrdp_write_mouse_event(s, &msg);

	status = xrdp_module_send(mod, Stream_Buffer(s), length);

	return status;
}

int xrdp_client_extended_mouse_event(xrdpModule* mod, DWORD flags, DWORD x, DWORD y)
{
	int length;
	int status;
	wStream* s;
	XRDP_MSG_EXTENDED_MOUSE_EVENT msg;

	msg.msgFlags = 0;
	msg.type = XRDP_CLIENT_EXTENDED_MOUSE_EVENT;

	msg.flags = flags;
	msg.x = x;
	msg.y = y;

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_extended_mouse_event(NULL, &msg);
	xrdp_write_extended_mouse_event(s, &msg);

	status = xrdp_module_send(mod, Stream_Buffer(s), length);

	return status;
}

int xup_recv_msg(xrdpModule* mod, wStream* s, XRDP_MSG_COMMON* common)
{
	int status = 0;

	switch (common->type)
	{
		case XRDP_SERVER_BEGIN_UPDATE:
			{
				XRDP_MSG_BEGIN_UPDATE msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->BeginUpdate(mod, &msg);
			}
			break;

		case XRDP_SERVER_END_UPDATE:
			{
				XRDP_MSG_END_UPDATE msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->EndUpdate(mod, &msg);
			}
			break;

		case XRDP_SERVER_OPAQUE_RECT:
			{
				XRDP_MSG_OPAQUE_RECT msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->OpaqueRect(mod, &msg);
			}
			break;

		case XRDP_SERVER_SCREEN_BLT:
			{
				XRDP_MSG_SCREEN_BLT msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->ScreenBlt(mod, &msg);
			}
			break;

		case XRDP_SERVER_PATBLT:
			{
				XRDP_MSG_PATBLT msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->PatBlt(mod, &msg);
			}
			break;

		case XRDP_SERVER_DSTBLT:
			{
				XRDP_MSG_DSTBLT msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->DstBlt(mod, &msg);
			}
			break;

		case XRDP_SERVER_PAINT_RECT:
			{
				int status;
				XRDP_MSG_PAINT_RECT msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));

				msg.fbSegmentId = 0;
				msg.framebuffer = NULL;

				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);

				if (msg.fbSegmentId)
					msg.framebuffer = &(mod->framebuffer);

				status = mod->server->PaintRect(mod, &msg);
			}
			break;

		case XRDP_SERVER_SET_CLIPPING_REGION:
			{
				XRDP_MSG_SET_CLIPPING_REGION msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->SetClippingRegion(mod, &msg);
			}
			break;

		case XRDP_SERVER_LINE_TO:
			{
				XRDP_MSG_LINE_TO msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->LineTo(mod, &msg);
			}
			break;

		case XRDP_SERVER_SET_POINTER:
			{
				XRDP_MSG_SET_POINTER msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->SetPointer(mod, &msg);
			}
			break;

		case XRDP_SERVER_CREATE_OFFSCREEN_SURFACE:
			{
				XRDP_MSG_CREATE_OFFSCREEN_SURFACE msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->CreateOffscreenSurface(mod, &msg);
			}
			break;

		case XRDP_SERVER_SWITCH_OFFSCREEN_SURFACE:
			{
				XRDP_MSG_SWITCH_OFFSCREEN_SURFACE msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->SwitchOffscreenSurface(mod, &msg);
			}
			break;

		case XRDP_SERVER_DELETE_OFFSCREEN_SURFACE:
			{
				XRDP_MSG_DELETE_OFFSCREEN_SURFACE msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->DeleteOffscreenSurface(mod, &msg);
			}
			break;

		case XRDP_SERVER_PAINT_OFFSCREEN_SURFACE:
			{
				XRDP_MSG_PAINT_OFFSCREEN_SURFACE msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->PaintOffscreenSurface(mod, &msg);
			}
			break;

		case XRDP_SERVER_WINDOW_NEW_UPDATE:
			{
				XRDP_MSG_WINDOW_NEW_UPDATE msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->WindowNewUpdate(mod, &msg);
			}
			break;

		case XRDP_SERVER_WINDOW_DELETE:
			{
				XRDP_MSG_WINDOW_DELETE msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->WindowDelete(mod, &msg);
			}
			break;

		case XRDP_SERVER_SHARED_FRAMEBUFFER:
			{
				XRDP_MSG_SHARED_FRAMEBUFFER msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_server_message_read(s, (XRDP_MSG_COMMON*) &msg);
				status = mod->server->SharedFramebuffer(mod, &msg);
			}
			break;

		default:
			printf("xup_recv_msg: unknown order type %d\n", common->type);
			status = 0;
			break;
	}

	return status;
}

int xup_recv(xrdpModule* mod)
{
	wStream* s;
	int index;
	int status;
	int position;

	s = mod->ReceiveStream;

	if (Stream_GetPosition(s) < 8)
	{
		status = xrdp_module_recv(mod, Stream_Pointer(s), 8 - Stream_GetPosition(s));

		if (status > 0)
			Stream_Seek(s, status);

		if (Stream_GetPosition(s) >= 8)
		{
			position = Stream_GetPosition(s);
			Stream_SetPosition(s, 0);

			Stream_Read_UINT32(s, mod->TotalLength);
			Stream_Read_UINT32(s, mod->TotalCount);

			Stream_SetPosition(s, position);

			Stream_EnsureCapacity(s, mod->TotalLength);
		}
	}

	if (Stream_GetPosition(s) >= 8)
	{
		status = xrdp_module_recv(mod, Stream_Pointer(s), mod->TotalLength - Stream_GetPosition(s));

		if (status > 0)
			Stream_Seek(s, status);
	}

	if (Stream_GetPosition(s) >= mod->TotalLength)
	{
		Stream_SetPosition(s, 8);

		for (index = 0; index < mod->TotalCount; index++)
		{
			XRDP_MSG_COMMON common;

			position = Stream_GetPosition(s);

			xrdp_read_common_header(s, &common);

			status = xup_recv_msg(mod, s, &common);

			if (status != 0)
			{
				break;
			}

			Stream_SetPosition(s, position + common.length);
		}

		Stream_SetPosition(s, 0);
		mod->TotalLength = 0;
		mod->TotalCount = 0;
	}

	return 0;
}

int xrdp_client_end(xrdpModule* mod)
{
	SetEvent(mod->StopEvent);

	return 0;
}

void* xrdp_client_thread(void* arg)
{
	int fps;
	DWORD status;
	DWORD nCount;
	HANDLE events[8];
	HANDLE PackTimer;
	LARGE_INTEGER due;
	xrdpModule* mod = (xrdpModule*) arg;

	fps = mod->fps;
	PackTimer = CreateWaitableTimer(NULL, TRUE, NULL);

	due.QuadPart = 0;
	SetWaitableTimer(PackTimer, &due, 1000 / fps, NULL, NULL, 0);

	nCount = 0;
	events[nCount++] = PackTimer;
	events[nCount++] = mod->StopEvent;
	events[nCount++] = mod->hClientPipe;

	while (1)
	{
		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		if (WaitForSingleObject(mod->StopEvent, 0) == WAIT_OBJECT_0)
		{
			break;
		}

		if (WaitForSingleObject(mod->hClientPipe, 0) == WAIT_OBJECT_0)
		{
			xup_recv(mod);
		}

		if (status == WAIT_OBJECT_0)
		{
			xrdp_message_server_queue_pack(mod);
		}

		if (mod->fps != fps)
		{
			fps = mod->fps;
			due.QuadPart = 0;
			SetWaitableTimer(PackTimer, &due, 1000 / fps, NULL, NULL, 0);
		}
	}

	CloseHandle(PackTimer);

	return NULL;
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

int xrdp_client_module_init(xrdpModule* mod)
{
	xrdpClientModule* client;

	client = (xrdpClientModule*) malloc(sizeof(xrdpClientModule));
	mod->client = client;

	if (client)
	{
		ZeroMemory(client, sizeof(xrdpClientModule));

		client->Connect = xrdp_client_connect;
		client->Start = xrdp_client_start;
		client->SynchronizeKeyboardEvent = xrdp_client_synchronize_keyboard_event;
		client->ScancodeKeyboardEvent = xrdp_client_scancode_keyboard_event;
		client->VirtualKeyboardEvent = xrdp_client_virtual_keyboard_event;
		client->UnicodeKeyboardEvent = xrdp_client_unicode_keyboard_event;
		client->MouseEvent = xrdp_client_mouse_event;
		client->ExtendedMouseEvent = xrdp_client_extended_mouse_event;
		client->End = xrdp_client_end;
		client->GetEventHandles = xrdp_client_get_event_handles;
		client->CheckEventHandles = xrdp_client_check_event_handles;
	}

	mod->SendStream = Stream_New(NULL, 8192);
	mod->ReceiveStream = Stream_New(NULL, 8192);

	mod->TotalLength = 0;
	mod->TotalCount = 0;

	mod->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	mod->ServerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) xrdp_client_thread,
			(void*) mod, CREATE_SUSPENDED, NULL);

	return 0;
}

int xrdp_client_module_uninit(xrdpModule* mod)
{
	SetEvent(mod->StopEvent);

	WaitForSingleObject(mod->ServerThread, INFINITE);
	CloseHandle(mod->ServerThread);

	Stream_Free(mod->SendStream, TRUE);
	Stream_Free(mod->ReceiveStream, TRUE);

	CloseHandle(mod->StopEvent);
	CloseHandle(mod->hClientPipe);

	return 0;
}
