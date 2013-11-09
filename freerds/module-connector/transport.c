/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * xrdp-ng interprocess communication protocol
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

#include <freerds/freerds.h>

#include <winpr/crt.h>
#include <winpr/file.h>
#include <winpr/pipe.h>
#include <winpr/path.h>
#include <winpr/print.h>
#include <winpr/thread.h>

#include "protocol.h"

#include "transport.h"

int freerds_named_pipe_read(HANDLE hNamedPipe, BYTE* data, DWORD length)
{
	BOOL fSuccess = FALSE;
	DWORD NumberOfBytesRead;
	DWORD TotalNumberOfBytesRead = 0;

	NumberOfBytesRead = 0;

	fSuccess = ReadFile(hNamedPipe, data, length, &NumberOfBytesRead, NULL);

	if (!fSuccess || (NumberOfBytesRead == 0))
	{
		return -1;
	}

	TotalNumberOfBytesRead += NumberOfBytesRead;
	length -= NumberOfBytesRead;
	data += NumberOfBytesRead;

	return TotalNumberOfBytesRead;
}

int freerds_named_pipe_write(HANDLE hNamedPipe, BYTE* data, DWORD length)
{
	BOOL fSuccess = FALSE;
	DWORD NumberOfBytesWritten;
	DWORD TotalNumberOfBytesWritten = 0;

	while (length > 0)
	{
		NumberOfBytesWritten = 0;

		fSuccess = WriteFile(hNamedPipe, data, length, &NumberOfBytesWritten, NULL);

		if (!fSuccess || (NumberOfBytesWritten == 0))
		{
			return -1;
		}

		TotalNumberOfBytesWritten += NumberOfBytesWritten;
		length -= NumberOfBytesWritten;
		data += NumberOfBytesWritten;
	}

	return TotalNumberOfBytesWritten;
}

void freerds_named_pipe_get_endpoint_name(DWORD id, const char* endpoint, char* dest, int len)
{
	sprintf_s(dest, len, "\\\\.\\pipe\\FreeRDS_%d_%s", (int) id, endpoint);
}

int freerds_named_pipe_clean(const char* pipeName)
{
	int status = 0;
	char* filename;

	filename = GetNamedPipeUnixDomainSocketFilePathA(pipeName);

	if (PathFileExistsA(filename))
	{
		DeleteFileA(filename);
		status = 1;
	}

	free(filename);
	return status;

}

int freerds_named_pipe_clean_endpoint(DWORD id, const char* endpoint)
{
	char pipeName[255];
	freerds_named_pipe_get_endpoint_name(id, endpoint, pipeName, 255);
	return freerds_named_pipe_clean(pipeName);
}

HANDLE freerds_named_pipe_connect(const char* pipeName, DWORD nTimeOut)
{
	HANDLE hNamedPipe;

	if (!WaitNamedPipeA(pipeName, nTimeOut))
	{
		fprintf(stderr, "WaitNamedPipe failure: %s\n", pipeName);
		return NULL;
	}

	hNamedPipe = CreateFileA(pipeName,
			GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if ((!hNamedPipe) || (hNamedPipe == INVALID_HANDLE_VALUE))
	{
		fprintf(stderr, "Failed to create named pipe %s\n", pipeName);
		return NULL;
	}

	return hNamedPipe;

}

HANDLE freerds_named_pipe_connect_endpoint(DWORD id, const char* endpoint, DWORD nTimeOut)
{
	char pipeName[255];
	freerds_named_pipe_get_endpoint_name(id, endpoint, pipeName, 255);
	return freerds_named_pipe_connect(pipeName, nTimeOut);
}

HANDLE freerds_named_pipe_create(const char* pipeName)
{
	HANDLE hNamedPipe;

	hNamedPipe = CreateNamedPipe(pipeName, PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES, PIPE_BUFFER_SIZE, PIPE_BUFFER_SIZE, 0, NULL);

	if ((!hNamedPipe) || (hNamedPipe == INVALID_HANDLE_VALUE))
	{
		fprintf(stderr, "CreateNamedPipe failure\n");
		return NULL;
	}

	return hNamedPipe;

}

HANDLE freerds_named_pipe_create_endpoint(DWORD id, const char* endpoint)
{
	char pipeName[255];
	freerds_named_pipe_get_endpoint_name(id, endpoint, pipeName, 255);
	return freerds_named_pipe_create(pipeName);
}

HANDLE freerds_named_pipe_accept(HANDLE hServerPipe)
{
	BOOL fConnected;
	DWORD dwPipeMode;
	HANDLE hClientPipe;

	fConnected = ConnectNamedPipe(hServerPipe, NULL);

	if (!fConnected)
		fConnected = (GetLastError() == ERROR_PIPE_CONNECTED);

	if (!fConnected)
	{
		return NULL;
	}

	hClientPipe = hServerPipe;

	dwPipeMode = PIPE_NOWAIT;
	SetNamedPipeHandleState(hClientPipe, &dwPipeMode, NULL, NULL);

	return hClientPipe;
}

int freerds_receive_server_message(rdsModuleConnector* connector, wStream* s, RDS_MSG_COMMON* common)
{
	int status = 0;
	rdsServerInterface* server;

	server = connector->server;

	switch (common->type)
	{
		case RDS_SERVER_BEGIN_UPDATE:
			{
				RDS_MSG_BEGIN_UPDATE msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->BeginUpdate(connector, &msg);
			}
			break;

		case RDS_SERVER_END_UPDATE:
			{
				RDS_MSG_END_UPDATE msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->EndUpdate(connector, &msg);
			}
			break;

		case RDS_SERVER_OPAQUE_RECT:
			{
				RDS_MSG_OPAQUE_RECT msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->OpaqueRect(connector, &msg);
			}
			break;

		case RDS_SERVER_SCREEN_BLT:
			{
				RDS_MSG_SCREEN_BLT msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->ScreenBlt(connector, &msg);
			}
			break;

		case RDS_SERVER_PATBLT:
			{
				RDS_MSG_PATBLT msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->PatBlt(connector, &msg);
			}
			break;

		case RDS_SERVER_DSTBLT:
			{
				RDS_MSG_DSTBLT msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->DstBlt(connector, &msg);
			}
			break;

		case RDS_SERVER_PAINT_RECT:
			{
				int status;
				RDS_MSG_PAINT_RECT msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));

				msg.fbSegmentId = 0;
				msg.framebuffer = NULL;

				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);

				if (msg.fbSegmentId)
					msg.framebuffer = &(connector->framebuffer);

				status = server->PaintRect(connector, &msg);
			}
			break;

		case RDS_SERVER_SET_CLIPPING_REGION:
			{
				RDS_MSG_SET_CLIPPING_REGION msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->SetClippingRegion(connector, &msg);
			}
			break;

		case RDS_SERVER_LINE_TO:
			{
				RDS_MSG_LINE_TO msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->LineTo(connector, &msg);
			}
			break;

		case RDS_SERVER_SET_POINTER:
			{
				RDS_MSG_SET_POINTER msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->SetPointer(connector, &msg);
			}
			break;

		case RDS_SERVER_SET_SYSTEM_POINTER:
			{
				RDS_MSG_SET_SYSTEM_POINTER msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->SetSystemPointer(connector, &msg);
			}
			break;

		case RDS_SERVER_CREATE_OFFSCREEN_SURFACE:
			{
				RDS_MSG_CREATE_OFFSCREEN_SURFACE msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->CreateOffscreenSurface(connector, &msg);
			}
			break;

		case RDS_SERVER_SWITCH_OFFSCREEN_SURFACE:
			{
				RDS_MSG_SWITCH_OFFSCREEN_SURFACE msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->SwitchOffscreenSurface(connector, &msg);
			}
			break;

		case RDS_SERVER_DELETE_OFFSCREEN_SURFACE:
			{
				RDS_MSG_DELETE_OFFSCREEN_SURFACE msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->DeleteOffscreenSurface(connector, &msg);
			}
			break;

		case RDS_SERVER_PAINT_OFFSCREEN_SURFACE:
			{
				RDS_MSG_PAINT_OFFSCREEN_SURFACE msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->PaintOffscreenSurface(connector, &msg);
			}
			break;

		case RDS_SERVER_WINDOW_NEW_UPDATE:
			{
				RDS_MSG_WINDOW_NEW_UPDATE msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->WindowNewUpdate(connector, &msg);
			}
			break;

		case RDS_SERVER_WINDOW_DELETE:
			{
				RDS_MSG_WINDOW_DELETE msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->WindowDelete(connector, &msg);
			}
			break;

		case RDS_SERVER_SHARED_FRAMEBUFFER:
			{
				RDS_MSG_SHARED_FRAMEBUFFER msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->SharedFramebuffer(connector, &msg);
			}
			break;

		case RDS_SERVER_LOGON_USER:
			{
				RDS_MSG_LOGON_USER msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->LogonUser(connector, &msg);
			}
			break;

		case RDS_SERVER_LOGOFF_USER:
			{
				RDS_MSG_LOGOFF_USER msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_server_message_read(s, (RDS_MSG_COMMON*) &msg);
				status = server->LogoffUser(connector, &msg);
			}
			break;

		default:
			status = 0;
			break;
	}

	return status;
}

int freerds_receive_client_message(rdsModuleConnector* connector, wStream* s, RDS_MSG_COMMON* common)
{
	int status = 0;
	rdsClientInterface* client;

	client = connector->client;

	switch (common->type)
	{
		case RDS_CLIENT_SYNCHRONIZE_KEYBOARD_EVENT:
			{
				RDS_MSG_SYNCHRONIZE_KEYBOARD_EVENT msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_read_synchronize_keyboard_event(s, &msg);
				status = client->SynchronizeKeyboardEvent(connector, msg.flags);
			}
			break;

		case RDS_CLIENT_SCANCODE_KEYBOARD_EVENT:
			{
				RDS_MSG_SCANCODE_KEYBOARD_EVENT msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_read_scancode_keyboard_event(s, &msg);
				status = client->ScancodeKeyboardEvent(connector, msg.flags, msg.code, msg.keyboardType);
			}
			break;

		case RDS_CLIENT_VIRTUAL_KEYBOARD_EVENT:
			{
				RDS_MSG_VIRTUAL_KEYBOARD_EVENT msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_read_virtual_keyboard_event(s, &msg);
				status = client->VirtualKeyboardEvent(connector, msg.flags, msg.code);
			}
			break;

		case RDS_CLIENT_UNICODE_KEYBOARD_EVENT:
			{
				RDS_MSG_UNICODE_KEYBOARD_EVENT msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_read_unicode_keyboard_event(s, &msg);
				status = client->UnicodeKeyboardEvent(connector, msg.flags, msg.code);
			}
			break;

		case RDS_CLIENT_MOUSE_EVENT:
			{
				RDS_MSG_MOUSE_EVENT msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_read_mouse_event(s, &msg);
				status = client->MouseEvent(connector, msg.flags, msg.x, msg.y);
			}
			break;

		case RDS_CLIENT_EXTENDED_MOUSE_EVENT:
			{
				RDS_MSG_EXTENDED_MOUSE_EVENT msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_read_extended_mouse_event(s, &msg);
				status = client->ExtendedMouseEvent(connector, msg.flags, msg.x, msg.y);
			}
			break;

		case RDS_CLIENT_VBLANK_EVENT:
			{
				RDS_MSG_VBLANK_EVENT msg;
				CopyMemory(&msg, common, sizeof(RDS_MSG_COMMON));
				freerds_read_vblank_event(s, &msg);
				if (client->VBlankEvent)
					status = client->VBlankEvent(connector);
				else
					status = 0;
			}
			break;

		default:
			status = 0;
			break;
	}

	return status;
}

int freerds_receive_message(rdsModuleConnector* connector, wStream* s, RDS_MSG_COMMON* common)
{
	if (connector->ServerMode)
		return freerds_receive_client_message(connector, s, common);
	else
		return freerds_receive_server_message(connector, s, common);
}

int freerds_transport_receive(rdsModuleConnector* connector)
{
	wStream* s;
	int status;
	int position;
	UINT32 length;
	RDS_MSG_COMMON common;

	s = connector->InboundStream;

	if (Stream_GetPosition(s) < RDS_ORDER_HEADER_LENGTH)
	{
		status = freerds_named_pipe_read(connector->hClientPipe, Stream_Pointer(s),
				RDS_ORDER_HEADER_LENGTH - Stream_GetPosition(s));

		if (status < 0)
			return -1;

		if (status > 0)
			Stream_Seek(s, status);
	}

	if (Stream_GetPosition(s) >= RDS_ORDER_HEADER_LENGTH)
	{
		length = freerds_peek_common_header_length(Stream_Buffer(s));

		if (length - Stream_GetPosition(s))
		{
			status = freerds_named_pipe_read(connector->hClientPipe, Stream_Pointer(s),
				length - Stream_GetPosition(s));

			if (status < 0)
				return -1;

			if (status > 0)
				Stream_Seek(s, status);
		}
	}

	if (Stream_GetPosition(s) >= RDS_ORDER_HEADER_LENGTH)
	{
		length = freerds_peek_common_header_length(Stream_Buffer(s));

		if (Stream_GetPosition(s) >= length)
		{
			position = Stream_GetPosition(s);

			Stream_SetPosition(s, 0);
			freerds_read_common_header(s, &common);

			status = freerds_receive_message(connector, s, &common);
			Stream_SetPosition(s, 0);
		}
	}

	return 0;
}
