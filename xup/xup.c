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

#include "arch.h"
#include "os_calls.h"
#include "defines.h"
#include "xrdp_rail.h"

#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include <avro.h>

#include <winpr/crt.h>
#include <winpr/stream.h>

#include <freerdp/freerdp.h>

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

	LIB_DEBUG(mod, "in lib_mod_connect");
	/* clear screen */
	server_begin_update(mod);
	server_set_fgcolor(mod, 0);
	server_fill_rect(mod, 0, 0, mod->width, mod->height);
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

	if (error == 0)
	{
		/* send version message */
		Stream_SetPosition(s, 0);
		Stream_Seek(s, 4);

		Stream_Write_UINT16(s, 103);
		Stream_Write_UINT32(s, 301);
		Stream_Write_UINT32(s, 0);
		Stream_Write_UINT32(s, 0);
		Stream_Write_UINT32(s, 0);
		Stream_Write_UINT32(s, 1);

		len = (int) (s->pointer - s->buffer);
		s->pointer = s->buffer;
		Stream_Write_UINT32(s, len);

		s->pointer = s->buffer + len;
		lib_send(mod, s->buffer, len);
	}

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

		len = (int) (s->pointer - s->buffer);
		s->pointer = s->buffer;
		Stream_Write_UINT32(s, len);

		s->pointer = s->buffer + len;
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

		len = (int) (s->pointer - s->buffer);
		s->pointer = s->buffer;
		Stream_Write_UINT32(s, len);

		s->pointer = s->buffer + len;
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

int lib_mod_event(xrdpModule* mod, int msg, tbus param1, tbus param2, tbus param3, tbus param4)
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

	len = (int) (s->pointer - s->buffer);
	s->pointer = s->buffer;

	Stream_Write_UINT32(s, len);

	s->pointer = s->buffer + len;
	rv = lib_send(mod, s->buffer, len);

	Stream_Free(s, TRUE);

	LIB_DEBUG(mod, "out lib_mod_event");

	return rv;
}

static int process_server_window_new_update(xrdpModule* mod, wStream* s)
{
	int flags;
	int window_id;
	int title_bytes;
	int index;
	int bytes;
	int rv;
	struct rail_window_state_order rwso;

	g_memset(&rwso, 0, sizeof(rwso));
	Stream_Read_UINT32(s, window_id);
	Stream_Read_UINT32(s, rwso.owner_window_id);
	Stream_Read_UINT32(s, rwso.style);
	Stream_Read_UINT32(s, rwso.extended_style);
	Stream_Read_UINT32(s, rwso.show_state);
	Stream_Read_UINT16(s, title_bytes);

	if (title_bytes > 0)
	{
		rwso.title_info = g_malloc(title_bytes + 1, 0);
		Stream_Read(s, rwso.title_info, title_bytes);
		rwso.title_info[title_bytes] = 0;
	}

	Stream_Read_UINT32(s, rwso.client_offset_x);
	Stream_Read_UINT32(s, rwso.client_offset_y);
	Stream_Read_UINT32(s, rwso.client_area_width);
	Stream_Read_UINT32(s, rwso.client_area_height);
	Stream_Read_UINT32(s, rwso.rp_content);
	Stream_Read_UINT32(s, rwso.root_parent_handle);
	Stream_Read_UINT32(s, rwso.window_offset_x);
	Stream_Read_UINT32(s, rwso.window_offset_y);
	Stream_Read_UINT32(s, rwso.window_client_delta_x);
	Stream_Read_UINT32(s, rwso.window_client_delta_y);
	Stream_Read_UINT32(s, rwso.window_width);
	Stream_Read_UINT32(s, rwso.window_height);
	Stream_Read_UINT16(s, rwso.num_window_rects);

	if (rwso.num_window_rects > 0)
	{
		bytes = sizeof(struct rail_window_rect) * rwso.num_window_rects;
		rwso.window_rects = (struct rail_window_rect *) g_malloc(bytes, 0);

		for (index = 0; index < rwso.num_window_rects; index++)
		{
			Stream_Read_UINT16(s, rwso.window_rects[index].left);
			Stream_Read_UINT16(s, rwso.window_rects[index].top);
			Stream_Read_UINT16(s, rwso.window_rects[index].right);
			Stream_Read_UINT16(s, rwso.window_rects[index].bottom);
		}
	}

	Stream_Read_UINT32(s, rwso.visible_offset_x);
	Stream_Read_UINT32(s, rwso.visible_offset_y);
	Stream_Read_UINT16(s, rwso.num_visibility_rects);

	if (rwso.num_visibility_rects > 0)
	{
		bytes = sizeof(struct rail_window_rect) * rwso.num_visibility_rects;
		rwso.visibility_rects = (struct rail_window_rect *) g_malloc(bytes, 0);

		for (index = 0; index < rwso.num_visibility_rects; index++)
		{
			Stream_Read_UINT16(s, rwso.visibility_rects[index].left);
			Stream_Read_UINT16(s, rwso.visibility_rects[index].top);
			Stream_Read_UINT16(s, rwso.visibility_rects[index].right);
			Stream_Read_UINT16(s, rwso.visibility_rects[index].bottom);
		}
	}

	Stream_Read_UINT32(s, flags);
	server_window_new_update(mod, window_id, &rwso, flags);
	rv = 0;
	g_free(rwso.title_info);
	g_free(rwso.window_rects);
	g_free(rwso.visibility_rects);

	return rv;
}

static int process_server_window_delete(xrdpModule* mod, wStream* s)
{
	int window_id;
	int rv;

	Stream_Read_UINT32(s, window_id);
	server_window_delete(mod, window_id);
	rv = 0;

	return rv;
}

static int process_server_set_pointer_ex(xrdpModule* mod, wStream* s)
{
	int rv;
	int x;
	int y;
	int bpp;
	int Bpp;
	char cur_data[32 * (32 * 4)];
	char cur_mask[32 * (32 / 8)];

	Stream_Read_INT16(s, x);
	Stream_Read_INT16(s, y);
	Stream_Read_UINT16(s, bpp);
	Bpp = (bpp == 0) ? 3 : (bpp + 7) / 8;
	Stream_Read(s, cur_data, 32 * (32 * Bpp));
	Stream_Read(s, cur_mask, 32 * (32 / 8));
	rv = server_set_pointer_ex(mod, x, y, cur_data, cur_mask, bpp);

	return rv;
}

static int process_server_paint_rect(xrdpModule* mod, wStream* s)
{
	int status;
	XRDP_MSG_PAINT_RECT msg;

	msg.fbSegmentId = 0;
	msg.framebuffer = NULL;

	Stream_Read_INT16(s, msg.nLeftRect);
	Stream_Read_INT16(s, msg.nTopRect);
	Stream_Read_UINT16(s, msg.nWidth);
	Stream_Read_UINT16(s, msg.nHeight);
	Stream_Read_UINT32(s, msg.bitmapDataLength);

	if (msg.bitmapDataLength)
	{
		Stream_GetPointer(s, msg.bitmapData);
		Stream_Seek(s, msg.bitmapDataLength);
	}
	else
	{
		Stream_Read_UINT32(s, msg.fbSegmentId);
		msg.framebuffer = &(mod->framebuffer);
	}

	Stream_Read_UINT16(s, msg.nWidth);
	Stream_Read_UINT16(s, msg.nHeight);
	Stream_Read_INT16(s, msg.nXSrc);
	Stream_Read_INT16(s, msg.nYSrc);

	status = server_paint_rect(mod, &msg);

	return status;
}

static int process_server_shared_framebuffer(xrdpModule* mod, wStream* s)
{
	int status = 0;
	XRDP_MSG_SHARED_FRAMEBUFFER msg;

	xrdp_read_shared_framebuffer(s, &msg);

	mod->framebuffer.fbWidth = msg.width;
	mod->framebuffer.fbHeight = msg.height;
	mod->framebuffer.fbScanline = msg.scanline;
	mod->framebuffer.fbSegmentId = msg.segmentId;
	mod->framebuffer.fbBitsPerPixel = msg.bitsPerPixel;
	mod->framebuffer.fbBytesPerPixel = msg.bytesPerPixel;

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

	return status;
}

static int lib_mod_process_orders(xrdpModule* mod, int type, wStream* s)
{
	int rv;
	int x;
	int y;
	int cx;
	int cy;
	int srcx;
	int srcy;
	int style;
	int x1;
	int y1;
	int x2;
	int y2;
	int rdpid;
	int hints;
	int mask;
	int width;
	int height;
	int fgcolor;
	int bgcolor;
	int opcode;
	char cur_data[32 * (32 * 3)];
	char cur_mask[32 * (32 / 8)];

	rv = 0;

	switch (type)
	{
		case XRDP_SERVER_BEGIN_UPDATE:
			rv = server_begin_update(mod);
			break;

		case XRDP_SERVER_END_UPDATE:
			rv = server_end_update(mod);
			break;

		case XRDP_SERVER_OPAQUE_RECT:
			Stream_Read_INT16(s, x);
			Stream_Read_INT16(s, y);
			Stream_Read_UINT16(s, cx);
			Stream_Read_UINT16(s, cy);
			rv = server_fill_rect(mod, x, y, cx, cy);
			break;

		case XRDP_SERVER_SCREEN_BLT:
			Stream_Read_INT16(s, x);
			Stream_Read_INT16(s, y);
			Stream_Read_UINT16(s, cx);
			Stream_Read_UINT16(s, cy);
			Stream_Read_INT16(s, srcx);
			Stream_Read_INT16(s, srcy);
			rv = server_screen_blt(mod, x, y, cx, cy, srcx, srcy);
			break;

		case XRDP_SERVER_PAINT_RECT:
			process_server_paint_rect(mod, s);
			break;

		case XRDP_SERVER_SET_CLIP:
			Stream_Read_INT16(s, x);
			Stream_Read_INT16(s, y);
			Stream_Read_UINT16(s, cx);
			Stream_Read_UINT16(s, cy);
			rv = server_set_clip(mod, x, y, cx, cy);
			break;

		case XRDP_SERVER_RESET_CLIP:
			rv = server_reset_clip(mod);
			break;

		case XRDP_SERVER_SET_FORECOLOR:
			Stream_Read_UINT32(s, fgcolor);
			rv = server_set_fgcolor(mod, fgcolor);
			break;

		case XRDP_SERVER_SET_BACKCOLOR:
			Stream_Read_UINT32(s, bgcolor);
			rv = server_set_bgcolor(mod, bgcolor);
			break;

		case XRDP_SERVER_SET_ROP2:
			Stream_Read_UINT16(s, opcode);
			rv = server_set_opcode(mod, opcode);
			break;

		case XRDP_SERVER_SET_PEN:
			Stream_Read_UINT16(s, style);
			Stream_Read_UINT16(s, width);
			rv = server_set_pen(mod, style, width);
			break;

		case XRDP_SERVER_LINE_TO:
			Stream_Read_INT16(s, x1);
			Stream_Read_INT16(s, y1);
			Stream_Read_INT16(s, x2);
			Stream_Read_INT16(s, y2);
			rv = server_draw_line(mod, x1, y1, x2, y2);
			break;

		case XRDP_SERVER_SET_POINTER:
			Stream_Read_INT16(s, x);
			Stream_Read_INT16(s, y);
			Stream_Read(s, cur_data, 32 * (32 * 3));
			Stream_Read(s, cur_mask, 32 * (32 / 8));
			rv = server_set_pointer(mod, x, y, cur_data, cur_mask);
			break;

		case XRDP_SERVER_SET_POINTER_EX:
			rv = process_server_set_pointer_ex(mod, s);
			break;

		case XRDP_SERVER_CREATE_OS_SURFACE:
			Stream_Read_UINT32(s, rdpid);
			Stream_Read_UINT16(s, width);
			Stream_Read_UINT16(s, height);
			rv = server_create_os_surface(mod, rdpid, width, height);
			break;

		case XRDP_SERVER_SWITCH_OS_SURFACE:
			Stream_Read_UINT32(s, rdpid);
			rv = server_switch_os_surface(mod, rdpid);
			break;

		case XRDP_SERVER_DELETE_OS_SURFACE:
			Stream_Read_UINT32(s, rdpid);
			rv = server_delete_os_surface(mod, rdpid);
			break;

		case XRDP_SERVER_MEMBLT:
			Stream_Read_INT16(s, x);
			Stream_Read_INT16(s, y);
			Stream_Read_UINT16(s, cx);
			Stream_Read_UINT16(s, cy);
			Stream_Read_UINT32(s, rdpid);
			Stream_Read_INT16(s, srcx);
			Stream_Read_INT16(s, srcy);
			rv = server_paint_rect_os(mod, x, y, cx, cy, rdpid, srcx, srcy);
			break;

		case XRDP_SERVER_SET_HINTS:
			Stream_Read_UINT32(s, hints);
			Stream_Read_UINT32(s, mask);
			rv = server_set_hints(mod, hints, mask);
			break;

		case XRDP_SERVER_WINDOW_NEW_UPDATE:
			rv = process_server_window_new_update(mod, s);
			break;

		case XRDP_SERVER_WINDOW_DELETE:
			rv = process_server_window_delete(mod, s);
			break;

		case XRDP_SERVER_SHARED_FRAMEBUFFER:
			rv = process_server_shared_framebuffer(mod, s);
			break;

		default:
			g_writeln("lib_mod_process_orders: unknown order type %d", type);
			rv = 0;
			break;
	}

	return rv;
}

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

/******************************************************************************/
/* return error */
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

		if (type == 1) /* original order list */
		{
			Stream_EnsureCapacity(s, len);
			Stream_SetPosition(s, 0);
			s->length = 0;

			rv = lib_recv(mod, s->buffer, len);

			if (rv == 0)
			{
				for (index = 0; index < num_orders; index++)
				{
					Stream_Read_UINT16(s, type);
					rv = lib_mod_process_orders(mod, type, s);

					if (rv != 0)
					{
						break;
					}
				}
			}
		}
		else if (type == 2) /* caps */
		{
			g_writeln("lib_mod_signal: type 2 len %d", len);

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
					Stream_Read_UINT16(s, len);

					switch (type)
					{
						default:
							g_writeln("lib_mod_signal: unknown cap type %d len %d",
									type, len);
							break;
					}

					s->pointer = phold + len;
				}

				lib_send_capabilities(mod);
			}
		}
		else if (type == 3) /* order list with len after type */
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

/******************************************************************************/
/* return error */
int lib_mod_end(xrdpModule* mod)
{
	return 0;
}

/******************************************************************************/
/* return error */
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

/******************************************************************************/
/* return error */
int lib_mod_get_wait_objs(xrdpModule* mod, tbus *read_objs, int *rcount, tbus *write_objs, int *wcount, int *timeout)
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

/******************************************************************************/
/* return error */
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

/******************************************************************************/
xrdpModule* xup_module_init(void)
{
	xrdpModule* mod;

	mod = (xrdpModule *) g_malloc(sizeof(xrdpModule), 1);
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

	return mod;
}

/******************************************************************************/
int xup_module_exit(xrdpModule* mod)
{
	if (mod == 0)
	{
		return 0;
	}

	g_delete_wait_obj_from_socket(mod->sck_obj);
	g_tcp_close(mod->sck);
	g_free(mod);

	return 0;
}
