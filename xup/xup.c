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
#include <sys/shm.h>
#include <sys/stat.h>

#include <avro.h>

#include <winpr/crt.h>
#include <winpr/stream.h>

#include <freerdp/freerdp.h>

int lib_send(xrdpModule* mod, unsigned char *data, int len);

const char CAPABILITIES_SCHEMA[] =
"{\"type\":\"record\",\
	\"name\":\"Capabilities\",\
	\"fields\":[\
		{\"name\": \"JPEG\", \"type\": \"boolean\"},\
		{\"name\": \"NSCodec\", \"type\": \"boolean\"},\
		{\"name\": \"RemoteFX\", \"type\": \"boolean\"},\
		{\"name\": \"OffscreenSupportLevel\", \"type\": \"int\"},\
		{\"name\": \"OffscreenCacheSize\", \"type\": \"int\"},\
		{\"name\": \"OffscreenCacheEntries\", \"type\": \"int\"},\
		{\"name\": \"RailSupportLevel\", \"type\": \"int\"},\
		{\"name\": \"PointerFlags\", \"type\": \"int\"}\
		]}";

static int lib_send_capabilities(xrdpModule* mod)
{
	size_t index;
	size_t length;
	char* buffer;

	avro_schema_t record_schema;
	avro_schema_from_json_literal(CAPABILITIES_SCHEMA, &record_schema);

	avro_value_iface_t* record_class = avro_generic_class_from_schema(record_schema);

	avro_value_t val;
	avro_generic_value_new(record_class, &val);

	avro_value_t field;

	avro_value_get_by_name(&val, "JPEG", &field, &index);
	avro_value_set_boolean(&field, mod->settings->JpegCodec);

	avro_value_get_by_name(&val, "NSCodec", &field, &index);
	avro_value_set_boolean(&field, mod->settings->NSCodec);

	avro_value_get_by_name(&val, "RemoteFX", &field, &index);
	avro_value_set_boolean(&field, mod->settings->RemoteFxCodec);

	avro_value_get_by_name(&val, "OffscreenSupportLevel", &field, &index);
	avro_value_set_int(&field, mod->settings->OffscreenSupportLevel);

	avro_value_get_by_name(&val, "OffscreenCacheSize", &field, &index);
	avro_value_set_int(&field, mod->settings->OffscreenCacheSize);

	avro_value_get_by_name(&val, "OffscreenCacheEntries", &field, &index);
	avro_value_set_int(&field, mod->settings->OffscreenCacheEntries);

	avro_value_get_by_name(&val, "RailSupportLevel", &field, &index);
	avro_value_set_int(&field, mod->settings->RemoteApplicationMode);

	avro_value_get_by_name(&val, "PointerFlags", &field, &index);
	avro_value_set_int(&field, mod->settings->ColorPointerFlag);

	avro_value_sizeof(&val, &length);

	buffer = (char*) malloc(length + 6);

	avro_writer_t writer = avro_writer_memory(&buffer[6], (int64_t) length);
	avro_value_write(writer, &val);

	avro_value_iface_decref(record_class);
	avro_schema_decref(record_schema);

        avro_writer_flush(writer);

        *((UINT32*) &buffer[0]) = (UINT32) length + 6;
        *((UINT16*) &buffer[4]) = 104;

	lib_send(mod, (BYTE*) buffer, (int) length + 6);

        avro_writer_free(writer);
	free(buffer);

	return 0;
}

int lib_recv(xrdpModule* mod, unsigned char *data, int len)
{
	int rcvd;

	if (mod->sck_closed)
	{
		return 1;
	}

	while (len > 0)
	{
		rcvd = g_tcp_recv(mod->sck, data, len, 0);

		if (rcvd == -1)
		{
			if (g_tcp_last_error_would_block(mod->sck))
			{
				if (server_is_term(mod))
				{
					return 1;
				}

				g_tcp_can_recv(mod->sck, 10);
			}
			else
			{
				return 1;
			}
		}
		else if (rcvd == 0)
		{
			mod->sck_closed = 1;
			return 1;
		}
		else
		{
			data += rcvd;
			len -= rcvd;
		}
	}

	return 0;
}

int lib_send(xrdpModule* mod, unsigned char *data, int len)
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

int lib_mod_start(xrdpModule* mod, int w, int h, int bpp)
{
	mod->width = w;
	mod->height = h;
	mod->bpp = bpp;
	return 0;
}

int lib_mod_connect(xrdpModule* mod)
{
	int error;
	int len;
	int i;
	int index;
	int use_uds;
	wStream* s;
	char con_port[256];
	XRDP_MSG_OPAQUE_RECT opaqueRect;

	LIB_DEBUG(mod, "in lib_mod_connect");

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
		LIB_DEBUG(mod, "out lib_mod_connect error");
		return 1;
	}

	if (g_strcmp(mod->ip, "") == 0)
	{
		server_msg(mod, "error - no ip set", 0);
		LIB_DEBUG(mod, "out lib_mod_connect error");
		return 1;
	}

	s = Stream_New(NULL, 8192);

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
			error = g_tcp_local_connect(mod->sck, con_port);
		}
		else
		{
			error = g_tcp_connect(mod->sck, mod->ip, con_port);
		}

		if (error == -1)
		{
			if (g_tcp_last_error_would_block(mod->sck))
			{
				error = 0;
				index = 0;

				while (!g_tcp_can_send(mod->sck, 100))
				{
					index++;

					if ((index >= 30) || server_is_term(mod))
					{
						server_msg(mod, "connect timeout", 0);
						error = 1;
						break;
					}
				}
			}
			else
			{
				server_msg(mod, "connect error", 0);
			}
		}

		if (error == 0)
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

	if (error == 0)
	{
		/* send screen size message */
		Stream_SetPosition(s, 0);
		Stream_Seek(s, 4);

		Stream_Write_UINT16(s, 103);
		Stream_Write_UINT32(s, 300);
		Stream_Write_UINT32(s, mod->width);
		Stream_Write_UINT32(s, mod->height);
		Stream_Write_UINT32(s, mod->bpp);
		Stream_Write_UINT32(s, mod->rfx);

		len = (int) Stream_GetPosition(s);
		Stream_SetPosition(s, 0);
		Stream_Write_UINT32(s, len);

		Stream_SetPosition(s, len);
		lib_send(mod, s->buffer, len);
	}

	if (error == 0)
	{
		/* send invalidate message */
		Stream_SetPosition(s, 0);
		Stream_Seek(s, 4);

		Stream_Write_UINT16(s, 103);
		Stream_Write_UINT32(s, 200);
		/* x and y */
		i = 0;
		Stream_Write_UINT32(s, i);
		/* width and height */
		i = ((mod->width & 0xFFFF) << 16) | mod->height;
		Stream_Write_UINT32(s, i);
		Stream_Write_UINT32(s, 0);
		Stream_Write_UINT32(s, 0);

		len = (int) Stream_GetPosition(s);
		Stream_SetPosition(s, 0);
		Stream_Write_UINT32(s, len);

		Stream_SetPosition(s, len);
		lib_send(mod, s->buffer, len);
	}

	Stream_Free(s, TRUE);

	if (error != 0)
	{
		server_msg(mod, "some problem", 0);
		LIB_DEBUG(mod, "out lib_mod_connect error");
		return 1;
	}
	else
	{
		server_msg(mod, "connected ok", 0);
		mod->sck_obj = g_create_wait_obj_from_socket(mod->sck, 0);
	}

	LIB_DEBUG(mod, "out lib_mod_connect");
	return 0;
}

int lib_mod_event(xrdpModule* mod, int msg, long param1, long param2, long param3, long param4)
{
	wStream* s;
	int len;
	int key;
	int rv;

	LIB_DEBUG(mod, "in lib_mod_event");

	s = Stream_New(NULL, 8192);

	if ((msg >= 15) && (msg <= 16)) /* key events */
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
					Stream_SetPosition(s, 0);
					Stream_Seek(s, 4);

					Stream_Write_UINT16(s, 103);
					Stream_Write_UINT32(s, 16); /* key up */
					Stream_Write_UINT32(s, 0);
					Stream_Write_UINT32(s, 65507); /* left control */
					Stream_Write_UINT32(s, 29); /* RDP scan code */
					Stream_Write_UINT32(s, 0xc000); /* flags */

					len = (int) (s->pointer - s->buffer);
					s->pointer = s->buffer;

					Stream_Write_UINT32(s, len);

					s->pointer = s->buffer + len;
					lib_send(mod, s->buffer, len);
				}
			}

			if (key == 65507) /* left control */
			{
				mod->shift_state = msg == 15;
			}
		}
	}

	Stream_SetPosition(s, 0);
	Stream_Seek(s, 4);

	Stream_Write_UINT16(s, 103);
	Stream_Write_UINT32(s, msg);
	Stream_Write_UINT32(s, param1);
	Stream_Write_UINT32(s, param2);
	Stream_Write_UINT32(s, param3);
	Stream_Write_UINT32(s, param4);

	len = (int) Stream_GetPosition(s);
	Stream_SetPosition(s, 0);

	Stream_Write_UINT32(s, len);

	Stream_SetPosition(s, len);
	rv = lib_send(mod, s->buffer, len);

	Stream_Free(s, TRUE);

	LIB_DEBUG(mod, "out lib_mod_event");

	return rv;
}

static int lib_mod_process_orders(xrdpModule* mod, int type, wStream* s)
{
	int status = 0;

	switch (type)
	{
		case XRDP_SERVER_BEGIN_UPDATE:
			{
				XRDP_MSG_BEGIN_UPDATE msg;
				xrdp_read_begin_update(s, &msg);
				status = server_begin_update(mod);
			}
			break;

		case XRDP_SERVER_END_UPDATE:
			{
				XRDP_MSG_END_UPDATE msg;
				xrdp_read_end_update(s, &msg);
				status = server_end_update(mod);
			}
			break;

		case XRDP_SERVER_OPAQUE_RECT:
			{
				XRDP_MSG_OPAQUE_RECT msg;
				xrdp_read_opaque_rect(s, &msg);
				status = server_opaque_rect(mod, &msg);
			}
			break;

		case XRDP_SERVER_SCREEN_BLT:
			{
				XRDP_MSG_SCREEN_BLT msg;
				xrdp_read_screen_blt(s, &msg);
				status = server_screen_blt(mod, &msg);
			}
			break;

		case XRDP_SERVER_PAINT_RECT:
			{
				int status;
				XRDP_MSG_PAINT_RECT msg;

				msg.fbSegmentId = 0;
				msg.framebuffer = NULL;

				xrdp_read_paint_rect(s, &msg);

				if (msg.fbSegmentId)
					msg.framebuffer = &(mod->framebuffer);

				status = server_paint_rect(mod, &msg);
			}
			break;

		case XRDP_SERVER_SET_CLIP:
			{
				XRDP_MSG_SET_CLIP msg;
				xrdp_read_set_clip(s, &msg);
				status = server_set_clip(mod, &msg);
			}
			break;

		case XRDP_SERVER_RESET_CLIP:
			{
				XRDP_MSG_RESET_CLIP msg;
				xrdp_read_reset_clip(s, &msg);
				status = server_reset_clip(mod);
			}
			break;

		case XRDP_SERVER_SET_FORECOLOR:
			{
				XRDP_MSG_SET_FORECOLOR msg;
				xrdp_read_set_forecolor(s, &msg);
				status = server_set_fgcolor(mod, msg.ForeColor);
			}
			break;

		case XRDP_SERVER_SET_BACKCOLOR:
			{
				XRDP_MSG_SET_BACKCOLOR msg;
				xrdp_read_set_backcolor(s, &msg);
				status = server_set_bgcolor(mod, msg.BackColor);
			}
			break;

		case XRDP_SERVER_SET_ROP2:
			{
				XRDP_MSG_SET_ROP2 msg;
				xrdp_read_set_rop2(s, &msg);
				status = server_set_opcode(mod, msg.bRop2);
			}
			break;

		case XRDP_SERVER_SET_PEN:
			{
				XRDP_MSG_SET_PEN msg;
				xrdp_read_set_pen(s, &msg);
				status = server_set_pen(mod, msg.PenStyle, msg.PenWidth);
			}
			break;

		case XRDP_SERVER_LINE_TO:
			{
				XRDP_MSG_LINE_TO msg;
				xrdp_read_line_to(s, &msg);
				status = server_draw_line(mod, msg.nXStart, msg.nYStart, msg.nXEnd, msg.nYStart);
			}
			break;

		case XRDP_SERVER_SET_POINTER:
			{
				XRDP_MSG_SET_POINTER msg;
				xrdp_read_set_pointer(s, &msg);
				status = server_set_pointer(mod, msg.xPos, msg.yPos, (char*) msg.xorMaskData, (char*) msg.andMaskData);
			}
			break;

		case XRDP_SERVER_SET_POINTER_EX:
			{
				XRDP_MSG_SET_POINTER_EX msg;
				xrdp_read_set_pointer_ex(s, &msg);
				status = server_set_pointer_ex(mod, msg.xPos, msg.yPos, (char*) msg.xorMaskData, (char*) msg.andMaskData, msg.xorBpp);
			}
			break;

		case XRDP_SERVER_CREATE_OS_SURFACE:
			{
				XRDP_MSG_CREATE_OS_SURFACE msg;
				xrdp_read_create_os_surface(s, &msg);
				status = server_create_os_surface(mod, msg.index, msg.width, msg.height);
			}
			break;

		case XRDP_SERVER_SWITCH_OS_SURFACE:
			{
				XRDP_MSG_SWITCH_OS_SURFACE msg;
				xrdp_read_switch_os_surface(s, &msg);
				status = server_switch_os_surface(mod, msg.index);
			}
			break;

		case XRDP_SERVER_DELETE_OS_SURFACE:
			{
				XRDP_MSG_DELETE_OS_SURFACE msg;
				xrdp_read_delete_os_surface(s, &msg);
				status = server_delete_os_surface(mod, msg.index);
			}
			break;

		case XRDP_SERVER_MEMBLT:
			{
				XRDP_MSG_MEMBLT msg;
				xrdp_read_memblt(s, &msg);
				status = server_paint_rect_os(mod, msg.nLeftRect, msg.nTopRect,
						msg.nWidth, msg.nHeight, msg.index, msg.nXSrc, msg.nYSrc);
			}
			break;

		case XRDP_SERVER_SET_HINTS:
			{
				XRDP_MSG_SET_HINTS msg;
				xrdp_read_set_hints(s, &msg);
				status = server_set_hints(mod, msg.hints, msg.mask);
			}
			break;

		case XRDP_SERVER_WINDOW_NEW_UPDATE:
			{
				XRDP_MSG_WINDOW_NEW_UPDATE msg;
				status = xrdp_read_window_new_update(s, &msg);
				server_window_new_update(mod, &msg);
			}
			break;

		case XRDP_SERVER_WINDOW_DELETE:
			{
				XRDP_MSG_WINDOW_DELETE msg;
				status = xrdp_read_window_delete(s, &msg);
				server_window_delete(mod, &msg);
			}
			break;

		case XRDP_SERVER_SHARED_FRAMEBUFFER:
			{
				XRDP_MSG_SHARED_FRAMEBUFFER msg;

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
			g_writeln("lib_mod_process_orders: unknown order type %d", type);
			status = 0;
			break;
	}

	return status;
}

int lib_mod_signal(xrdpModule* mod)
{
	wStream* s;
	int num_orders;
	int index;
	int rv;
	int len;
	int type;
	BYTE* phold;

	LIB_DEBUG(mod, "in lib_mod_signal");

	s = Stream_New(NULL, 8192);

	rv = lib_recv(mod, s->buffer, 8);

	if (rv == 0)
	{
		Stream_Read_UINT16(s, type);
		Stream_Read_UINT16(s, num_orders);
		Stream_Read_UINT32(s, len);

		printf("lib_mod_signal: type: %d num_orders: %d length: %d\n", type, num_orders, len);

		if (type == 3)
		{
			Stream_EnsureCapacity(s, len);
			Stream_SetPosition(s, 0);
			s->length = 0;

			rv = lib_recv(mod, s->buffer, len);

			if (rv == 0)
			{
				for (index = 0; index < num_orders; index++)
				{
					phold = s->pointer;

					Stream_Read_UINT16(s, type);
					Stream_Read_UINT32(s, len);

					rv = lib_mod_process_orders(mod, type, s);

					if (rv != 0)
					{
						break;
					}

					s->pointer = phold + len;
				}
			}
		}
		else
		{
			g_writeln("unknown type %d", type);
		}
	}

	Stream_Free(s, TRUE);

	LIB_DEBUG(mod, "out lib_mod_signal");

	return rv;
}

int lib_mod_end(xrdpModule* mod)
{
	return 0;
}

int lib_mod_set_param(xrdpModule* mod, char *name, char *value)
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

int lib_mod_get_wait_objs(xrdpModule* mod, LONG_PTR *read_objs, int *rcount, LONG_PTR *write_objs, int *wcount, int *timeout)
{
	int i;

	i = *rcount;

	if (mod != 0)
	{
		if (mod->sck_obj != 0)
		{
			read_objs[i++] = mod->sck_obj;
		}
	}

	*rcount = i;
	return 0;
}

int lib_mod_check_wait_objs(xrdpModule* mod)
{
	int rv;

	rv = 0;

	if (mod != 0)
	{
		if (mod->sck_obj != 0)
		{
			if (g_is_wait_obj_set(mod->sck_obj))
			{
				rv = lib_mod_signal(mod);
			}
		}
	}

	return rv;
}

xrdpModule* xup_module_init(void)
{
	xrdpModule* mod;

	mod = (xrdpModule*) malloc(sizeof(xrdpModule));

	if (mod)
	{
		ZeroMemory(mod, sizeof(xrdpModule));
		mod->size = sizeof(xrdpModule);
		mod->version = 2;
		mod->handle = (long) mod;
		mod->mod_connect = lib_mod_connect;
		mod->mod_start = lib_mod_start;
		mod->mod_event = lib_mod_event;
		mod->mod_signal = lib_mod_signal;
		mod->mod_end = lib_mod_end;
		mod->mod_set_param = lib_mod_set_param;
		mod->mod_get_wait_objs = lib_mod_get_wait_objs;
		mod->mod_check_wait_objs = lib_mod_check_wait_objs;
	}

	return mod;
}

int xup_module_exit(xrdpModule* mod)
{
	if (!mod)
		return 0;

	g_delete_wait_obj_from_socket(mod->sck_obj);
	g_tcp_close(mod->sck);
	g_free(mod);

	return 0;
}
