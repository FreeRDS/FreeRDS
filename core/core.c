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

#include <winpr/crt.h>

#include "core.h"

struct xrdp_session* libxrdp_init(tbus id, struct trans* trans)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_exit(struct xrdp_session* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_disconnect(struct xrdp_session* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_process_incomming(struct xrdp_session* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_process_data(struct xrdp_session* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_send_palette(struct xrdp_session* session, int* palette)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_send_bell(struct xrdp_session* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_send_bitmap(struct xrdp_session* session, int width, int height, int bpp, char* data, int x, int y, int cx, int cy)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_send_pointer(struct xrdp_session* session, int cache_idx, char* data, char* mask, int x, int y, int bpp)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_set_pointer(struct xrdp_session* session, int cache_idx)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_init(struct xrdp_session* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send(struct xrdp_session* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_force_send(struct xrdp_session* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_rect(struct xrdp_session* session, int x, int y,
		int cx, int cy, int color, struct xrdp_rect* rect)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_screen_blt(struct xrdp_session* session, int x, int y,
		int cx, int cy, int srcx, int srcy, int rop, struct xrdp_rect* rect)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_pat_blt(struct xrdp_session* session, int x, int y,
		int cx, int cy, int rop, int bg_color, int fg_color,
		struct xrdp_brush* brush, struct xrdp_rect* rect)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_dest_blt(struct xrdp_session* session,
		int x, int y, int cx, int cy, int rop, struct xrdp_rect* rect)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_line(struct xrdp_session* session, int mix_mode,
		int startx, int starty,	int endx, int endy, int rop,
		int bg_color, struct xrdp_pen* pen, struct xrdp_rect* rect)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_mem_blt(struct xrdp_session* session, int cache_id,
		int color_table, int x, int y, int cx, int cy, int rop, int srcx,
		int srcy, int cache_idx, struct xrdp_rect* rect)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_text(struct xrdp_session* session,
		int font, int flags, int mixmode,
		int fg_color, int bg_color,
		int clip_left, int clip_top,
		int clip_right, int clip_bottom,
		int box_left, int box_top,
		int box_right, int box_bottom,
		int x, int y, char* data, int data_len,
		struct xrdp_rect* rect)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_palette(struct xrdp_session* session, int* palette, int cache_id)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_raw_bitmap(struct xrdp_session* session,
		int width, int height, int bpp, char* data,
		int cache_id, int cache_idx)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_bitmap(struct xrdp_session* session,
		int width, int height, int bpp, char* data,
		int cache_id, int cache_idx)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_font(struct xrdp_session* session,
		struct xrdp_font_char* font_char, int font_index, int char_index)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_reset(struct xrdp_session* session, int width, int height, int bpp)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_raw_bitmap2(struct xrdp_session* session,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_bitmap2(struct xrdp_session* session,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx, int hints)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_bitmap3(struct xrdp_session* session,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx, int hints)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_query_channel(struct xrdp_session* session, int index,
		char* channel_name, int* channel_flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_get_channel_id(struct xrdp_session* session, char* name)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_send_to_channel(struct xrdp_session* session, int channel_id,
		char* data, int data_len, int total_data_len, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_brush(struct xrdp_session* session, int width, int height,
		int bpp, int type, int size, char* data, int cache_id)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_create_os_surface(struct xrdp_session* session, int id,
		int width, int height, struct list* del_list)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_switch_os_surface(struct xrdp_session* session, int id)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_window_new_update(struct xrdp_session* session, int window_id,
		struct rail_window_state_order* window_state, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_window_delete(struct xrdp_session* session, int window_id)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_window_icon(struct xrdp_session* session, int window_id,
		int cache_entry, int cache_id, struct rail_icon_info* icon_info, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_window_cached_icon(struct xrdp_session* session, int window_id,
		int cache_entry, int cache_id, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_notify_new_update(struct xrdp_session* session,
		int window_id, int notify_id, struct rail_notify_state_order* notify_state, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_notify_delete(struct xrdp_session* session, int window_id, int notify_id)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_monitored_desktop(struct xrdp_session* session, struct rail_monitored_desktop_order* mdo, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}
