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
#include <freerdp/codec/rfx.h>
#include <freerdp/codec/nsc.h>

#include <freerdp/channels/wtsvc.h>
#include <freerdp/server/cliprdr.h>
#include <freerdp/server/rdpdr.h>
#include <freerdp/server/rdpsnd.h>
#include <freerdp/server/drdynvc.h>

#include <winpr/crt.h>
#include <winpr/stream.h>

#include <freerds/freerds.h>

#include "xrdp.h"

struct xrdp_brush
{
	int x_orgin;
	int y_orgin;
	int style;
	char pattern[8];
};
typedef struct xrdp_brush xrdpBrush;

struct xrdp_rect
{
	int left;
	int top;
	int right;
	int bottom;
};
typedef struct xrdp_rect xrdpRect;

struct xrdp_session
{
	rdpContext context;

	long id;
	rdsModule* mod;
	HANDLE Thread;
	HANDLE TermEvent;
	freerdp_peer* client;
	rdpSettings* settings;

	BOOL codecMode;
	int bytesPerPixel;

	wStream* bs;
	wStream* bts;

	wStream* rfx_s;
	RFX_CONTEXT* rfx_context;

	wStream* nsc_s;
	NSC_CONTEXT* nsc_context;

	UINT32 frameId;
	wListDictionary* FrameList;

	WTSVirtualChannelManager* vcm;
	CliprdrServerContext* cliprdr;
	RdpdrServerContext* rdpdr;
	RdpsndServerContext* rdpsnd;
	DrdynvcServerContext* drdynvc;
};

FREERDP_API int libxrdp_session_init(xrdpSession* session, rdpSettings* settings);
FREERDP_API void libxrdp_session_uninit(xrdpSession* session);

FREERDP_API int libxrdp_send_palette(xrdpSession* session, int* palette);

FREERDP_API int libxrdp_send_bell(xrdpSession* session);

FREERDP_API int libxrdp_send_bitmap_update(xrdpSession* session, int bpp, XRDP_MSG_PAINT_RECT* msg);

FREERDP_API int libxrdp_set_pointer(xrdpSession* session, XRDP_MSG_SET_POINTER* msg);

FREERDP_API int libxrdp_orders_begin_paint(xrdpSession* session);

FREERDP_API int libxrdp_orders_end_paint(xrdpSession* session);

FREERDP_API int libxrdp_orders_rect(xrdpSession* session, int x, int y,
		int cx, int cy, int color, xrdpRect* rect);

FREERDP_API int libxrdp_orders_screen_blt(xrdpSession* session, int x, int y,
		int cx, int cy, int srcx, int srcy, int rop, xrdpRect* rect);

FREERDP_API int libxrdp_orders_pat_blt(xrdpSession* session, int x, int y,
		int cx, int cy, int rop, int bg_color, int fg_color,
		xrdpBrush* brush, xrdpRect* rect);

FREERDP_API int libxrdp_orders_dest_blt(xrdpSession* session,
		int x, int y, int cx, int cy, int rop, xrdpRect* rect);

FREERDP_API int libxrdp_orders_line(xrdpSession* session, XRDP_MSG_LINE_TO* msg, xrdpRect* rect);

FREERDP_API int libxrdp_orders_mem_blt(xrdpSession* session, int cache_id,
		int color_table, int x, int y, int cx, int cy, int rop, int srcx,
		int srcy, int cache_idx, xrdpRect* rect);

FREERDP_API int libxrdp_orders_text(xrdpSession* session, XRDP_MSG_GLYPH_INDEX* msg, xrdpRect* rect);

FREERDP_API int libxrdp_orders_send_palette(xrdpSession* session, int* palette, int cache_id);

FREERDP_API int libxrdp_orders_send_raw_bitmap(xrdpSession* session,
		int width, int height, int bpp, char* data,
		int cache_id, int cache_idx);

FREERDP_API int libxrdp_orders_send_bitmap(xrdpSession* session,
		int width, int height, int bpp, char* data,
		int cache_id, int cache_idx);

FREERDP_API int libxrdp_orders_send_font(xrdpSession* session, XRDP_MSG_CACHE_GLYPH* msg);

FREERDP_API int libxrdp_reset(xrdpSession* session, XRDP_MSG_RESET* msg);

FREERDP_API int libxrdp_orders_send_raw_bitmap2(xrdpSession* session,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx);

FREERDP_API int libxrdp_orders_send_bitmap2(xrdpSession* session,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx, int hints);

FREERDP_API int libxrdp_orders_send_bitmap3(xrdpSession* session,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx, int hints);

FREERDP_API int libxrdp_orders_send_brush(xrdpSession* session, int width, int height,
		int bpp, int type, int size, char* data, int cache_id);

FREERDP_API int libxrdp_orders_send_create_os_surface(xrdpSession* session,
		CREATE_OFFSCREEN_BITMAP_ORDER* createOffscreenBitmap);

FREERDP_API int libxrdp_orders_send_switch_os_surface(xrdpSession* session, int id);

FREERDP_API int libxrdp_send_surface_bits(xrdpSession* session, int bpp, XRDP_MSG_PAINT_RECT* msg);

FREERDP_API int libxrdp_orders_send_frame_marker(xrdpSession* session, UINT32 action, UINT32 id);

FREERDP_API int libxrdp_window_new_update(xrdpSession* session, XRDP_MSG_WINDOW_NEW_UPDATE* msg);

FREERDP_API int libxrdp_window_delete(xrdpSession* session, XRDP_MSG_WINDOW_DELETE* msg);

#endif /* FREERDP_XRDP_NG_CORE_H */
