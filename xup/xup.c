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
 * libxup main file
 */

#include "xup.h"

#include "os_calls.h"
#include "defines.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/stat.h>

#include <sys/ioctl.h>
#include <sys/socket.h>

#include <avro.h>

#include <winpr/crt.h>
#include <winpr/synch.h>
#include <winpr/stream.h>

#include <freerdp/freerdp.h>

int lib_send_all(xrdpModule* mod, unsigned char *data, int len);

static int lib_send_capabilities(xrdpModule* mod)
{
	wStream* s;
	int length;
	rdpSettings* settings;
	XRDP_MSG_CAPABILITIES msg;

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	msg.flags = 0;
	msg.type = XRDP_CLIENT_CAPABILITIES;

	settings = mod->settings;

	msg.DesktopWidth = settings->DesktopWidth;
	msg.DesktopHeight= settings->DesktopHeight;
	msg.ColorDepth = settings->ColorDepth;

	msg.SupportedCodecs = 0;

	if (settings->JpegCodec)
		msg.SupportedCodecs |= XRDP_CODEC_JPEG;

	if (settings->NSCodec)
		msg.SupportedCodecs |= XRDP_CODEC_NSCODEC;

	if (settings->RemoteFxCodec)
		msg.SupportedCodecs |= XRDP_CODEC_REMOTEFX;

	msg.OffscreenSupportLevel = settings->OffscreenSupportLevel;
	msg.OffscreenCacheSize = settings->OffscreenCacheSize;
	msg.OffscreenCacheEntries = settings->OffscreenCacheEntries;
	msg.RailSupportLevel = settings->RemoteApplicationMode;
	msg.PointerFlags = settings->ColorPointerFlag;

	length = xrdp_write_capabilities(NULL, &msg);

	xrdp_write_capabilities(s, &msg);

	lib_send_all(mod, Stream_Buffer(s), length);

	return 0;
}

int lib_recv(xrdpModule* mod, BYTE* data, int length)
{
	int status;

	if (mod->sck_closed)
		return -1;

	status = recv(mod->sck, data, length, 0);

	if (status < 0)
	{
		if (g_tcp_last_error_would_block(mod->sck))
		{
			if (server_is_term(mod))
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}
	}
	else if (status == 0)
	{
		mod->sck_closed = 1;
		return -1;
	}

	return status;
}

int lib_send_all(xrdpModule* mod, unsigned char *data, int len)
{
	int sent;

	if (mod->sck_closed)
	{
		return 1;
	}

	while (len > 0)
	{
		sent = g_tcp_send(mod->sck, data, len, 0);

		if (sent == -1)
		{
			if (g_tcp_last_error_would_block(mod->sck))
			{
				if (server_is_term(mod))
				{
					return 1;
				}

				g_tcp_can_send(mod->sck, 10);
			}
			else
			{
				return 1;
			}
		}
		else if (sent == 0)
		{
			mod->sck_closed = 1;
			return 1;
		}
		else
		{
			data += sent;
			len -= sent;
		}
	}

	return 0;
}

int x11rdp_xrdp_client_start(xrdpModule* mod, int w, int h, int bpp)
{
	mod->width = w;
	mod->height = h;
	mod->bpp = bpp;
	return 0;
}

int x11rdp_xrdp_client_connect(xrdpModule* mod)
{
	int i;
	int index;
	int status;
	int length;
	int use_uds;
	wStream* s;
	char con_port[256];
	XRDP_MSG_OPAQUE_RECT opaqueRect;

	opaqueRect.nTopRect = 0;
	opaqueRect.nLeftRect = 0;
	opaqueRect.nWidth = mod->width;
	opaqueRect.nHeight = mod->height;

	/* clear screen */
	server_begin_update(mod);
	server_set_fgcolor(mod, 0);
	server_opaque_rect(mod, &opaqueRect);
	server_end_update(mod);

	server_msg(mod, "started connecting", 0);

	/* only support 8, 15, 16, and 24 bpp connections from rdp client */
	if (mod->bpp != 8 && mod->bpp != 15 && mod->bpp != 16 && mod->bpp != 24 && mod->bpp != 32)
	{
		server_msg(mod, "error - only supporting 8, 15, 16, 24 and 32 bpp rdp connections", 0);
		LIB_DEBUG(mod, "x11rdp_xrdp_client_connect error");
		return 1;
	}

	if (g_strcmp(mod->ip, "") == 0)
	{
		server_msg(mod, "error - no ip set", 0);
		LIB_DEBUG(mod, "x11rdp_xrdp_client_connect error");
		return 1;
	}

	g_sprintf(con_port, "%s", mod->port);
	use_uds = 0;

	if (con_port[0] == '/')
	{
		use_uds = 1;
	}

	mod->sck_closed = 0;
	i = 0;

	while (1)
	{
		if (use_uds)
		{
			mod->sck = g_tcp_local_socket();
		}
		else
		{
			mod->sck = g_tcp_socket();
			g_tcp_set_non_blocking(mod->sck);
			g_tcp_set_no_delay(mod->sck);
		}

		server_msg(mod, "connecting...", 0);

		if (use_uds)
		{
			status = g_tcp_local_connect(mod->sck, con_port);
		}
		else
		{
			status = g_tcp_connect(mod->sck, mod->ip, con_port);
		}

		if (status == -1)
		{
			if (g_tcp_last_error_would_block(mod->sck))
			{
				status = 0;
				index = 0;

				while (!g_tcp_can_send(mod->sck, 100))
				{
					index++;

					if ((index >= 30) || server_is_term(mod))
					{
						server_msg(mod, "connect timeout", 0);
						status = 1;
						break;
					}
				}
			}
			else
			{
				server_msg(mod, "connect error", 0);
			}
		}

		if (status == 0)
		{
			break;
		}

		g_tcp_close(mod->sck);
		mod->sck = 0;
		i++;

		if (i >= 4)
		{
			server_msg(mod, "connection problem, giving up", 0);
			break;
		}

		g_sleep(250);
	}

	lib_send_capabilities(mod);

	if (status == 0)
	{
		RECTANGLE_16 rect;
		XRDP_MSG_REFRESH_RECT msg;

		msg.flags = 0;
		msg.numberOfAreas = 1;
		msg.areasToRefresh = &rect;

		rect.left = 0;
		rect.top = 0;
		rect.right = mod->settings->DesktopWidth - 1;
		rect.bottom = mod->settings->DesktopHeight - 1;

		s = mod->SendStream;
		Stream_SetPosition(s, 0);

		length = xrdp_write_refresh_rect(NULL, &msg);

		xrdp_write_refresh_rect(s, &msg);

		lib_send_all(mod, Stream_Buffer(s), length);
	}

	if (status != 0)
	{
		server_msg(mod, "some problem", 0);
		LIB_DEBUG(mod, "x11rdp_xrdp_client_connect error");
		return 1;
	}
	else
	{
		server_msg(mod, "connected ok", 0);
		mod->SocketEvent = CreateFileDescriptorEvent(NULL, TRUE, FALSE, mod->sck);
	}

	return 0;
}

int x11rdp_xrdp_client_event(xrdpModule* mod, int subtype, long param1, long param2, long param3, long param4)
{
	wStream* s;
	int length;
	int key;
	int status;
	XRDP_MSG_EVENT msg;

	LIB_DEBUG(mod, "in lib_mod_event");

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	if ((subtype >= 15) && (subtype <= 16)) /* key events */
	{
		key = param2;

		if (key > 0)
		{
			if (key == 65027) /* altgr */
			{
				if (mod->shift_state)
				{
					g_writeln("special");
					/* fix for mstsc sending left control down with altgr */
					/* control down / up
					 msg param1 param2 param3 param4
					 15  0      65507  29     0
					 16  0      65507  29     49152 */

					msg.flags = 0;
					msg.type = XRDP_CLIENT_EVENT;

					msg.subType = 16; /* key up */
					msg.param1 = 0;
					msg.param2 = 65507; /* left control */
					msg.param3 = 29; /* RDP scan code */
					msg.param4 = 0xc000; /* flags */

					Stream_SetPosition(s, 0);

					length = xrdp_write_event(NULL, &msg);
					xrdp_write_event(s, &msg);

					status = lib_send_all(mod, Stream_Buffer(s), length);
				}
			}

			if (key == 65507) /* left control */
			{
				mod->shift_state = subtype == 15;
			}
		}
	}

	msg.flags = 0;
	msg.type = XRDP_CLIENT_EVENT;

	msg.subType = subtype;
	msg.param1 = param1;
	msg.param2 = param2;
	msg.param3 = param3;
	msg.param4 = param4;

	Stream_SetPosition(s, 0);

	length = xrdp_write_event(NULL, &msg);
	xrdp_write_event(s, &msg);

	status = lib_send_all(mod, Stream_Buffer(s), length);

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
				xrdp_read_begin_update(s, &msg);
				status = server_begin_update(mod);
			}
			break;

		case XRDP_SERVER_END_UPDATE:
			{
				XRDP_MSG_END_UPDATE msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_read_end_update(s, &msg);
				status = server_end_update(mod);
			}
			break;

		case XRDP_SERVER_OPAQUE_RECT:
			{
				XRDP_MSG_OPAQUE_RECT msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_read_opaque_rect(s, &msg);
				status = server_opaque_rect(mod, &msg);
			}
			break;

		case XRDP_SERVER_SCREEN_BLT:
			{
				XRDP_MSG_SCREEN_BLT msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_read_screen_blt(s, &msg);
				status = server_screen_blt(mod, &msg);
			}
			break;

		case XRDP_SERVER_PAINT_RECT:
			{
				int status;
				XRDP_MSG_PAINT_RECT msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));

				msg.fbSegmentId = 0;
				msg.framebuffer = NULL;

				xrdp_read_paint_rect(s, &msg);

				if (msg.fbSegmentId)
					msg.framebuffer = &(mod->framebuffer);

				status = server_paint_rect(mod, &msg);
			}
			break;

		case XRDP_SERVER_SET_CLIPPING_REGION:
			{
				XRDP_MSG_SET_CLIPPING_REGION msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_read_set_clipping_region(s, &msg);

				if (msg.bNullRegion)
					status = server_reset_clip(mod);
				else
					status = server_set_clip(mod, &msg);
			}
			break;

		case XRDP_SERVER_SET_FORECOLOR:
			{
				XRDP_MSG_SET_FORECOLOR msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_read_set_forecolor(s, &msg);
				status = server_set_fgcolor(mod, msg.ForeColor);
			}
			break;

		case XRDP_SERVER_SET_ROP2:
			{
				XRDP_MSG_SET_ROP2 msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_read_set_rop2(s, &msg);
				status = server_set_opcode(mod, msg.bRop2);
			}
			break;

		case XRDP_SERVER_SET_PEN:
			{
				XRDP_MSG_SET_PEN msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_read_set_pen(s, &msg);
				status = server_set_pen(mod, msg.PenStyle, msg.PenWidth);
			}
			break;

		case XRDP_SERVER_LINE_TO:
			{
				XRDP_MSG_LINE_TO msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_read_line_to(s, &msg);
				status = server_draw_line(mod, msg.nXStart, msg.nYStart, msg.nXEnd, msg.nYStart);
			}
			break;

		case XRDP_SERVER_SET_POINTER:
			{
				XRDP_MSG_SET_POINTER msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_read_set_pointer(s, &msg);
				status = server_set_pointer(mod, &msg);
			}
			break;

		case XRDP_SERVER_CREATE_OS_SURFACE:
			{
				XRDP_MSG_CREATE_OS_SURFACE msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_read_create_os_surface(s, &msg);
				status = server_create_os_surface(mod, msg.index, msg.width, msg.height);
			}
			break;

		case XRDP_SERVER_SWITCH_OS_SURFACE:
			{
				XRDP_MSG_SWITCH_OS_SURFACE msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_read_switch_os_surface(s, &msg);
				status = server_switch_os_surface(mod, msg.index);
			}
			break;

		case XRDP_SERVER_DELETE_OS_SURFACE:
			{
				XRDP_MSG_DELETE_OS_SURFACE msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_read_delete_os_surface(s, &msg);
				status = server_delete_os_surface(mod, msg.index);
			}
			break;

		case XRDP_SERVER_MEMBLT:
			{
				XRDP_MSG_MEMBLT msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				xrdp_read_memblt(s, &msg);
				status = server_paint_rect_os(mod, msg.nLeftRect, msg.nTopRect,
						msg.nWidth, msg.nHeight, msg.index, msg.nXSrc, msg.nYSrc);
			}
			break;

		case XRDP_SERVER_WINDOW_NEW_UPDATE:
			{
				XRDP_MSG_WINDOW_NEW_UPDATE msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				status = xrdp_read_window_new_update(s, &msg);
				server_window_new_update(mod, &msg);
			}
			break;

		case XRDP_SERVER_WINDOW_DELETE:
			{
				XRDP_MSG_WINDOW_DELETE msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));
				status = xrdp_read_window_delete(s, &msg);
				server_window_delete(mod, &msg);
			}
			break;

		case XRDP_SERVER_SHARED_FRAMEBUFFER:
			{
				XRDP_MSG_SHARED_FRAMEBUFFER msg;
				CopyMemory(&msg, common, sizeof(XRDP_MSG_COMMON));

				status = xrdp_read_shared_framebuffer(s, &msg);

				mod->framebuffer.fbWidth = msg.width;
				mod->framebuffer.fbHeight = msg.height;
				mod->framebuffer.fbScanline = msg.scanline;
				mod->framebuffer.fbSegmentId = msg.segmentId;
				mod->framebuffer.fbBitsPerPixel = msg.bitsPerPixel;
				mod->framebuffer.fbBytesPerPixel = msg.bytesPerPixel;

				printf("received shared framebuffer message: mod->framebuffer.fbAttached: %d msg.attach: %d\n",
						mod->framebuffer.fbAttached, msg.attach);

				if (!mod->framebuffer.fbAttached && msg.attach)
				{
					mod->framebuffer.fbSharedMemory = (BYTE*) shmat(mod->framebuffer.fbSegmentId, 0, 0);
					mod->framebuffer.fbAttached = TRUE;

					printf("attached segment %d to %p\n",
							mod->framebuffer.fbSegmentId, mod->framebuffer.fbSharedMemory);
				}

				if (mod->framebuffer.fbAttached && !msg.attach)
				{
					shmdt(mod->framebuffer.fbSharedMemory);
					mod->framebuffer.fbAttached = FALSE;
					mod->framebuffer.fbSharedMemory = 0;
				}
			}
			break;

		default:
			g_writeln("lib_mod_process_orders: unknown order type %d", common->type);
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
		status = lib_recv(mod, Stream_Pointer(s), 8 - Stream_GetPosition(s));

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
		status = lib_recv(mod, Stream_Pointer(s), mod->TotalLength - Stream_GetPosition(s));

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

int x11rdp_xrdp_client_end(xrdpModule* mod)
{
	return 0;
}

int x11rdp_xrdp_client_set_param(xrdpModule* mod, char *name, char *value)
{
	if (g_strcasecmp(name, "username") == 0)
	{
		g_strncpy(mod->username, value, 255);
	}
	else if (g_strcasecmp(name, "password") == 0)
	{
		g_strncpy(mod->password, value, 255);
	}
	else if (g_strcasecmp(name, "ip") == 0)
	{
		g_strncpy(mod->ip, value, 255);
	}
	else if (g_strcasecmp(name, "port") == 0)
	{
		g_strncpy(mod->port, value, 255);
	}
	else if (g_strcasecmp(name, "rfx") == 0)
	{
		mod->rfx = g_atoi(value);
		g_writeln("mod->rfx = %d",mod->rfx);
	}
	else if (g_strcasecmp(name, "settings") == 0)
	{
		mod->settings = (rdpSettings*) value;
	}

	return 0;
}

int x11rdp_xrdp_client_get_event_handles(xrdpModule* mod, HANDLE* events, DWORD* nCount)
{
	if (mod)
	{
		if (mod->SocketEvent)
		{
			events[*nCount] = mod->SocketEvent;
			(*nCount)++;
		}
	}

	return 0;
}

int x11rdp_xrdp_client_check_event_handles(xrdpModule* mod)
{
	int status = 0;

	if (!mod)
		return 0;

	if (!mod->SocketEvent)
		return 0;

	if (WaitForSingleObject(mod->SocketEvent, 0) == WAIT_OBJECT_0)
	{
		status = xup_recv(mod);
	}

	return status;
}

int xup_module_init(xrdpModule* mod)
{
	xrdpClientModule* client;

	client = (xrdpClientModule*) malloc(sizeof(xrdpClientModule));
	mod->client = client;

	if (client)
	{
		ZeroMemory(client, sizeof(xrdpClientModule));

		client->Connect = x11rdp_xrdp_client_connect;
		client->Start = x11rdp_xrdp_client_start;
		client->Event = x11rdp_xrdp_client_event;
		client->End = x11rdp_xrdp_client_end;
		client->SetParam = x11rdp_xrdp_client_set_param;
		client->GetEventHandles = x11rdp_xrdp_client_get_event_handles;
		client->CheckEventHandles = x11rdp_xrdp_client_check_event_handles;
	}

	mod->SendStream = Stream_New(NULL, 8192);
	mod->ReceiveStream = Stream_New(NULL, 8192);

	mod->TotalLength = 0;
	mod->TotalCount = 0;

	return 0;
}

int xup_module_exit(xrdpModule* mod)
{
	Stream_Free(mod->SendStream, TRUE);
	Stream_Free(mod->ReceiveStream, TRUE);

	g_tcp_close(mod->sck);

	return 0;
}
