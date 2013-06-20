/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP X11 Server
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

#ifndef FREERDP_XRDP_NG_CORE_H
#define FREERDP_XRDP_NG_CORE_H

#include <freerdp/api.h>
#include <freerdp/freerdp.h>

/*
 * Phase 1: Export same interface as libxrdp and adapt internally to FreeRDP
 */

#include "xrdp_rail.h"

struct xrdp_brush
{
	int x_orgin;
	int y_orgin;
	int style;
	char pattern[8];
};
typedef struct xrdp_brush xrdpBrush;

struct xrdp_pen
{
	int style;
	int width;
	int color;
};
typedef struct xrdp_pen xrdpPen;

struct xrdp_font_char
{
	int offset;
	int baseline;
	int width;
	int height;
	int incby;
	char* data;
};
typedef struct xrdp_font_char xrdpFontChar;

struct xrdp_rect
{
	int left;
	int top;
	int right;
	int bottom;
};
typedef struct xrdp_rect xrdpRect;

#include "arch.h"
#include "parse.h"
#include "trans.h"
#include "xrdp_constants.h"
#include "defines.h"
#include "os_calls.h"
#include "list.h"
#include "file.h"

typedef struct xrdp_session xrdpSession;

struct xrdp_session
{
	long id;
	struct trans* trans;
	int (*callback)(long id, int msg, long param1, long param2, long param3, long param4);
	void* rdp;
	void* orders;
	int up_and_running;
	int (*is_term)(void);

	/* FreeRDP */
	rdpContext* context;
	freerdp_peer* client;
	rdpSettings* settings;

	wStream* bs;
	wStream* bts;
};

FREERDP_API xrdpSession* libxrdp_session_new();
FREERDP_API void libxrdp_session_free(xrdpSession* session);

FREERDP_API int libxrdp_send_palette(xrdpSession* session, int* palette);

FREERDP_API int libxrdp_send_bell(xrdpSession* session);

FREERDP_API int libxrdp_send_bitmap(xrdpSession* session, int width, int height, int bpp, char* data, int x, int y, int cx, int cy);

FREERDP_API int libxrdp_send_pointer(xrdpSession* session, int cache_idx, char* data, char* mask, int x, int y, int bpp);

FREERDP_API int libxrdp_set_pointer(xrdpSession* session, int cache_idx);

FREERDP_API int libxrdp_orders_init(xrdpSession* session);

FREERDP_API int libxrdp_orders_send(xrdpSession* session);

FREERDP_API int libxrdp_orders_force_send(xrdpSession* session);

FREERDP_API int libxrdp_orders_rect(xrdpSession* session, int x, int y,
		int cx, int cy, int color, xrdpRect* rect);

FREERDP_API int libxrdp_orders_screen_blt(xrdpSession* session, int x, int y,
		int cx, int cy, int srcx, int srcy, int rop, xrdpRect* rect);

FREERDP_API int libxrdp_orders_pat_blt(xrdpSession* session, int x, int y,
		int cx, int cy, int rop, int bg_color, int fg_color,
		struct xrdp_brush* brush, xrdpRect* rect);

FREERDP_API int libxrdp_orders_dest_blt(xrdpSession* session,
		int x, int y, int cx, int cy, int rop, xrdpRect* rect);

FREERDP_API int libxrdp_orders_line(xrdpSession* session, int mix_mode,
		int startx, int starty,	int endx, int endy, int rop,
		int bg_color, struct xrdp_pen* pen, xrdpRect* rect);

FREERDP_API int libxrdp_orders_mem_blt(xrdpSession* session, int cache_id,
		int color_table, int x, int y, int cx, int cy, int rop, int srcx,
		int srcy, int cache_idx, xrdpRect* rect);

FREERDP_API int libxrdp_orders_text(xrdpSession* session,
		int font, int flags, int mixmode,
		int fg_color, int bg_color,
		int clip_left, int clip_top,
		int clip_right, int clip_bottom,
		int box_left, int box_top,
		int box_right, int box_bottom,
		int x, int y, char* data, int data_len,
		xrdpRect* rect);

FREERDP_API int libxrdp_orders_send_palette(xrdpSession* session, int* palette, int cache_id);

FREERDP_API int libxrdp_orders_send_raw_bitmap(xrdpSession* session,
		int width, int height, int bpp, char* data,
		int cache_id, int cache_idx);

FREERDP_API int libxrdp_orders_send_bitmap(xrdpSession* session,
		int width, int height, int bpp, char* data,
		int cache_id, int cache_idx);

FREERDP_API int libxrdp_orders_send_font(xrdpSession* session,
		xrdpFontChar* font_char, int font_index, int char_index);

FREERDP_API int libxrdp_reset(xrdpSession* session, int width, int height, int bpp);

FREERDP_API int libxrdp_orders_send_raw_bitmap2(xrdpSession* session,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx);

FREERDP_API int libxrdp_orders_send_bitmap2(xrdpSession* session,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx, int hints);

FREERDP_API int libxrdp_orders_send_bitmap3(xrdpSession* session,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx, int hints);

FREERDP_API int libxrdp_query_channel(xrdpSession* session, int index,
		char* channel_name, int* channel_flags);

FREERDP_API int libxrdp_get_channel_id(xrdpSession* session, char* name);

FREERDP_API int libxrdp_send_to_channel(xrdpSession* session, int channel_id,
		char* data, int data_len, int total_data_len, int flags);

FREERDP_API int libxrdp_orders_send_brush(xrdpSession* session, int width, int height,
		int bpp, int type, int size, char* data, int cache_id);

FREERDP_API int libxrdp_orders_send_create_os_surface(xrdpSession* session, int id,
		int width, int height, struct list* del_list);

FREERDP_API int libxrdp_orders_send_switch_os_surface(xrdpSession* session, int id);

FREERDP_API int libxrdp_window_new_update(xrdpSession* session, int window_id,
		struct rail_window_state_order* window_state, int flags);

FREERDP_API int libxrdp_window_delete(xrdpSession* session, int window_id);

FREERDP_API int libxrdp_window_icon(xrdpSession* session, int window_id,
		int cache_entry, int cache_id, struct rail_icon_info* icon_info, int flags);

FREERDP_API int libxrdp_window_cached_icon(xrdpSession* session, int window_id,
		int cache_entry, int cache_id, int flags);

FREERDP_API int libxrdp_notify_new_update(xrdpSession* session,
		int window_id, int notify_id, struct rail_notify_state_order* notify_state, int flags);

FREERDP_API int libxrdp_notify_delete(xrdpSession* session,
		int window_id, int notify_id);

FREERDP_API int libxrdp_monitored_desktop(xrdpSession* session,
		struct rail_monitored_desktop_order* mdo, int flags);


#endif /* FREERDP_XRDP_NG_CORE_H */
