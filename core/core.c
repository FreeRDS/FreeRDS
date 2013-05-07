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

#include <freerdp/freerdp.h>
#include <freerdp/listener.h>

#include "core.h"

xrdpSession* libxrdp_init(tbus id, struct trans* trans)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_exit(xrdpSession* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_disconnect(xrdpSession* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_process_incomming(xrdpSession* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_process_data(xrdpSession* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_send_palette(xrdpSession* session, int* palette)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_send_bell(xrdpSession* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_send_bitmap(xrdpSession* session, int width, int height, int bpp, char* data, int x, int y, int cx, int cy)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_send_pointer(xrdpSession* session, int cache_idx, char* data, char* mask, int x, int y, int bpp)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_set_pointer(xrdpSession* session, int cache_idx)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_init(xrdpSession* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send(xrdpSession* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_force_send(xrdpSession* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_rect(xrdpSession* session, int x, int y,
		int cx, int cy, int color, struct xrdp_rect* rect)
{
	OPAQUE_RECT_ORDER opaqueRect;
	rdpPrimaryUpdate* primary = session->client->update->primary;

	printf("%s\n", __FUNCTION__);

	opaqueRect.nLeftRect = x;
	opaqueRect.nTopRect = y;
	opaqueRect.nWidth = cx;
	opaqueRect.nHeight = cy;
	opaqueRect.color = color;

	primary->OpaqueRect(session->context, &opaqueRect);

	return 0;
}

int libxrdp_orders_screen_blt(xrdpSession* session, int x, int y,
		int cx, int cy, int srcx, int srcy, int rop, struct xrdp_rect* rect)
{
	SCRBLT_ORDER scrblt;
	rdpPrimaryUpdate* primary = session->client->update->primary;

	printf("%s\n", __FUNCTION__);

	scrblt.nLeftRect = x;
	scrblt.nTopRect = y;
	scrblt.nWidth = cx;
	scrblt.nHeight = cy;
	scrblt.bRop = rop;
	scrblt.nXSrc = srcx;
	scrblt.nYSrc = srcy;

	primary->ScrBlt(session->context, &scrblt);

	return 0;
}

int libxrdp_orders_pat_blt(xrdpSession* session, int x, int y,
		int cx, int cy, int rop, int bg_color, int fg_color,
		struct xrdp_brush* brush, struct xrdp_rect* rect)
{
	PATBLT_ORDER patblt;
	//rdpPrimaryUpdate* primary = session->client->update->primary;

	printf("%s\n", __FUNCTION__);

	patblt.nLeftRect = x;
	patblt.nTopRect = y;
	patblt.nWidth = cx;
	patblt.nHeight = cy;
	patblt.bRop = (UINT32) rop;
	patblt.backColor = (UINT32) bg_color;
	patblt.foreColor = (UINT32) fg_color;

	patblt.brush.x = brush->x_orgin;
	patblt.brush.y = brush->y_orgin;
	patblt.brush.style = brush->style;
	patblt.brush.data = patblt.brush.p8x8;
	CopyMemory(patblt.brush.data, brush->pattern, 8);
	patblt.brush.hatch = patblt.brush.data[0];

	//primary->PatBlt(session->context, &patblt);

	return 0;
}

int libxrdp_orders_dest_blt(xrdpSession* session,
		int x, int y, int cx, int cy, int rop, struct xrdp_rect* rect)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_line(xrdpSession* session, int mix_mode,
		int startx, int starty,	int endx, int endy, int rop,
		int bg_color, struct xrdp_pen* pen, struct xrdp_rect* rect)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_mem_blt(xrdpSession* session, int cache_id,
		int color_table, int x, int y, int cx, int cy, int rop, int srcx,
		int srcy, int cache_idx, struct xrdp_rect* rect)
{
	MEMBLT_ORDER memblt;
	//rdpPrimaryUpdate* primary = session->client->update->primary;

	printf("%s\n", __FUNCTION__);

	memblt.nLeftRect = x;
	memblt.nTopRect = y;
	memblt.nWidth = cx;
	memblt.nHeight = cy;
	memblt.bRop = rop;
	memblt.nXSrc = srcx;
	memblt.nYSrc = srcy;
	memblt.cacheId = cache_id;
	memblt.cacheIndex = cache_idx;

	//primary->MemBlt(session->context, &memblt);

	return 0;
}

int libxrdp_orders_text(xrdpSession* session,
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

int libxrdp_orders_send_palette(xrdpSession* session, int* palette, int cache_id)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_raw_bitmap(xrdpSession* session,
		int width, int height, int bpp, char* data,
		int cache_id, int cache_idx)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_bitmap(xrdpSession* session,
		int width, int height, int bpp, char* data,
		int cache_id, int cache_idx)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_font(xrdpSession* session,
		struct xrdp_font_char* font_char, int font_index, int char_index)
{
	CACHE_GLYPH_V2_ORDER cache_glyph_v2;
	//rdpSecondaryUpdate* secondary = session->client->update->secondary;

	printf("%s\n", __FUNCTION__);

	cache_glyph_v2.cacheId = font_index;
	cache_glyph_v2.cGlyphs = 1;
	cache_glyph_v2.glyphData[0].cacheIndex = char_index;
	cache_glyph_v2.glyphData[0].x = font_char->offset;
	cache_glyph_v2.glyphData[0].y = font_char->baseline;
	cache_glyph_v2.glyphData[0].cx = font_char->width;
	cache_glyph_v2.glyphData[0].cy = font_char->height;
	cache_glyph_v2.glyphData[0].aj = (BYTE*) font_char->data;

	//secondary->CacheGlyphV2(session->context, &cache_glyph_v2);

	return 0;
}

int libxrdp_reset(xrdpSession* session, int width, int height, int bpp)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_raw_bitmap2(xrdpSession* session,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx)
{
	int bytesPerPixel;
	CACHE_BITMAP_V2_ORDER cache_bitmap_v2;
	rdpSecondaryUpdate* secondary = session->client->update->secondary;

	printf("%s\n", __FUNCTION__);

	cache_bitmap_v2.bitmapBpp = bpp;
	cache_bitmap_v2.bitmapWidth = width;
	cache_bitmap_v2.bitmapHeight = height;
	cache_bitmap_v2.bitmapDataStream = (BYTE*) data;
	cache_bitmap_v2.cacheId = cache_id;
	cache_bitmap_v2.cacheIndex = cache_idx;
	cache_bitmap_v2.compressed = FALSE;

	bytesPerPixel = (bpp + 7) / 8;
	cache_bitmap_v2.bitmapLength = width * height * bytesPerPixel;

	secondary->CacheBitmapV2(session->context, &cache_bitmap_v2);

	return 0;
}

int libxrdp_orders_send_bitmap2(xrdpSession* session,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx, int hints)
{
	int bytesPerPixel;
	CACHE_BITMAP_V2_ORDER cache_bitmap_v2;
	rdpSecondaryUpdate* secondary = session->client->update->secondary;

	printf("%s\n", __FUNCTION__);

	cache_bitmap_v2.bitmapBpp = bpp;
	cache_bitmap_v2.bitmapWidth = width;
	cache_bitmap_v2.bitmapHeight = height;
	cache_bitmap_v2.bitmapDataStream = (BYTE*) data;
	cache_bitmap_v2.cacheId = cache_id;
	cache_bitmap_v2.cacheIndex = cache_idx;
	cache_bitmap_v2.compressed = FALSE;

	bytesPerPixel = (bpp + 7) / 8;
	cache_bitmap_v2.bitmapLength = width * height * bytesPerPixel;

	secondary->CacheBitmapV2(session->context, &cache_bitmap_v2);

	return 0;
}

int libxrdp_orders_send_bitmap3(xrdpSession* session,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx, int hints)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_query_channel(xrdpSession* session, int index,
		char* channel_name, int* channel_flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_get_channel_id(xrdpSession* session, char* name)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_send_to_channel(xrdpSession* session, int channel_id,
		char* data, int data_len, int total_data_len, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_brush(xrdpSession* session, int width, int height,
		int bpp, int type, int size, char* data, int cache_id)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_create_os_surface(xrdpSession* session, int id,
		int width, int height, struct list* del_list)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_orders_send_switch_os_surface(xrdpSession* session, int id)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_window_new_update(xrdpSession* session, int window_id,
		struct rail_window_state_order* window_state, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_window_delete(xrdpSession* session, int window_id)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_window_icon(xrdpSession* session, int window_id,
		int cache_entry, int cache_id, struct rail_icon_info* icon_info, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_window_cached_icon(xrdpSession* session, int window_id,
		int cache_entry, int cache_id, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_notify_new_update(xrdpSession* session,
		int window_id, int notify_id, struct rail_notify_state_order* notify_state, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_notify_delete(xrdpSession* session, int window_id, int notify_id)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_monitored_desktop(xrdpSession* session, struct rail_monitored_desktop_order* mdo, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}
