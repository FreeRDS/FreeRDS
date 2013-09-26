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

int xrdp_client_receive_message(xrdpModule* mod, wStream* s, XRDP_MSG_COMMON* common)
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

int xrdp_client_receive(xrdpModule* mod)
{
	wStream* s;
	int index;
	int status;
	int position;

	s = mod->ReceiveStream;

	if (Stream_GetPosition(s) < 8)
	{
		status = freerds_named_pipe_read(mod->hClientPipe, Stream_Pointer(s), 8 - Stream_GetPosition(s));

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
		status = freerds_named_pipe_read(mod->hClientPipe, Stream_Pointer(s), mod->TotalLength - Stream_GetPosition(s));

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

			status = xrdp_client_receive_message(mod, s, &common);

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
			xrdp_client_receive(mod);
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
