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
 * module interface
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/stat.h>

#include "log.h"

#include "xrdp.h"

int xrdp_server_begin_update(xrdpModule* mod, XRDP_MSG_BEGIN_UPDATE* msg)
{
	xrdpWm* wm;
	xrdpPainter* p;

	wm = (xrdpWm*) (mod->wm);
	p = xrdp_painter_create(wm, wm->session);
	xrdp_painter_begin_update(p);
	mod->painter = (long) p;

	return 0;
}

int xrdp_server_end_update(xrdpModule* mod, XRDP_MSG_END_UPDATE* msg)
{
	xrdpPainter* p;

	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	xrdp_painter_end_update(p);
	xrdp_painter_delete(p);
	mod->painter = 0;

	return 0;
}

int xrdp_server_beep(xrdpModule* mod, XRDP_MSG_BEEP* msg)
{
	xrdpWm* wm;

	wm = (xrdpWm*) (mod->wm);
	libxrdp_send_bell(wm->session);

	return 0;
}

int xrdp_server_is_terminated(xrdpModule* mod)
{
	return g_is_term();
}

int xrdp_server_opaque_rect(xrdpModule* mod, XRDP_MSG_OPAQUE_RECT* msg)
{
	xrdpWm* wm;
	xrdpPainter* p;

	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	wm = (xrdpWm*) (mod->wm);

	p->fg_color = msg->color;
	xrdp_painter_fill_rect(p, wm->target_surface, msg->nLeftRect, msg->nTopRect, msg->nWidth, msg->nHeight);

	return 0;
}

int xrdp_server_screen_blt(xrdpModule* mod, XRDP_MSG_SCREEN_BLT* msg)
{
	xrdpWm* wm;
	xrdpPainter* p;

	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	wm = (xrdpWm*) (mod->wm);

	p->rop = 0xCC;
	xrdp_painter_copy(p, wm->screen, wm->target_surface, msg->nLeftRect, msg->nTopRect,
			msg->nWidth, msg->nHeight, msg->nXSrc, msg->nYSrc);

	return 0;
}

int xrdp_server_paint_rect(xrdpModule* mod, XRDP_MSG_PAINT_RECT* msg)
{
	int bpp;
	xrdpWm* wm;
	xrdpBitmap* b;
	xrdpPainter* p;

	wm = (xrdpWm*) (mod->wm);

	if (msg->fbSegmentId)
		bpp = msg->framebuffer->fbBitsPerPixel;
	else
		bpp = wm->screen->bpp;

	if (wm->session->codecMode)
	{
		libxrdp_send_surface_bits(wm->session, bpp, msg);
	}
	else
	{
		p = (xrdpPainter*) (mod->painter);

		if (!p)
			return 0;

		b = xrdp_bitmap_create_with_data(msg->nWidth, msg->nHeight, bpp, (char*) msg->bitmapData, wm);

		xrdp_painter_copy(p, b, wm->target_surface, msg->nLeftRect, msg->nTopRect,
				msg->nWidth, msg->nHeight, msg->nXSrc, msg->nYSrc);

		xrdp_bitmap_delete(b);
	}

	return 0;
}

int xrdp_server_patblt(xrdpModule* mod, XRDP_MSG_PATBLT* msg)
{
	xrdpWm* wm;
	xrdpPainter* p;

	wm = (xrdpWm*) (mod->wm);
	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	p->rop = msg->bRop;
	p->fg_color = msg->foreColor;
	p->pen.color = p->fg_color;
	p->bg_color = msg->backColor;

	p->brush.x_orgin = msg->brush.x;
	p->brush.y_orgin = msg->brush.y;
	p->brush.style = msg->brush.style;
	CopyMemory(p->brush.pattern, msg->brush.p8x8, 8);

	xrdp_painter_patblt(p, wm->target_surface, msg->nLeftRect, msg->nTopRect, msg->nWidth, msg->nHeight);

	p->rop = 0xCC;

	return 0;
}

int xrdp_server_dstblt(xrdpModule* mod, XRDP_MSG_DSTBLT* msg)
{
	xrdpWm* wm;
	xrdpPainter* p;

	wm = (xrdpWm*) (mod->wm);
	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	xrdp_painter_dstblt(p, wm->target_surface, msg);

	return 0;
}

int xrdp_server_set_pointer(xrdpModule* mod, XRDP_MSG_SET_POINTER* msg)
{
	xrdpWm* wm = (xrdpWm*) (mod->wm);

	xrdp_wm_pointer(wm, msg);

	return 0;
}

int xrdp_server_set_palette(xrdpModule* mod, XRDP_MSG_SET_PALETTE* msg)
{
	xrdpWm* wm = (xrdpWm*) (mod->wm);

	CopyMemory(wm->palette, msg->palette, 256 * sizeof(UINT32));
	libxrdp_send_palette(wm->session, wm->palette);

	return 0;
}

int xrdp_server_set_clipping_region(xrdpModule* mod, XRDP_MSG_SET_CLIPPING_REGION* msg)
{
	xrdpPainter* p;

	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	if (!msg->bNullRegion)
		return xrdp_painter_set_clip(p, msg->nLeftRect, msg->nTopRect, msg->nWidth, msg->nHeight);
	else
		return xrdp_painter_clr_clip(p);
}

int xrdp_server_line_to(xrdpModule* mod, XRDP_MSG_LINE_TO* msg)
{
	xrdpWm* wm;
	xrdpPainter* p;

	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	wm = (xrdpWm*) (mod->wm);

	return xrdp_painter_line(p, wm->target_surface, msg);
}

int xrdp_server_cache_glyph(xrdpModule* mod, XRDP_MSG_CACHE_GLYPH* msg)
{
	return libxrdp_orders_send_font(((xrdpWm*) mod->wm)->session, msg);
}

int xrdp_server_glyph_index(xrdpModule* mod, XRDP_MSG_GLYPH_INDEX* msg)
{
	xrdpWm* wm;
	xrdpPainter* p;

	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	wm = (xrdpWm*) (mod->wm);

	return xrdp_painter_draw_text2(p, wm->target_surface, msg);
}

int xrdp_server_shared_framebuffer(xrdpModule* mod, XRDP_MSG_SHARED_FRAMEBUFFER* msg)
{
	mod->framebuffer.fbWidth = msg->width;
	mod->framebuffer.fbHeight = msg->height;
	mod->framebuffer.fbScanline = msg->scanline;
	mod->framebuffer.fbSegmentId = msg->segmentId;
	mod->framebuffer.fbBitsPerPixel = msg->bitsPerPixel;
	mod->framebuffer.fbBytesPerPixel = msg->bytesPerPixel;

	printf("received shared framebuffer message: mod->framebuffer.fbAttached: %d msg->attach: %d\n",
			mod->framebuffer.fbAttached, msg->attach);

	if (!mod->framebuffer.fbAttached && msg->attach)
	{
		mod->framebuffer.fbSharedMemory = (BYTE*) shmat(mod->framebuffer.fbSegmentId, 0, 0);
		mod->framebuffer.fbAttached = TRUE;

		printf("attached segment %d to %p\n",
				mod->framebuffer.fbSegmentId, mod->framebuffer.fbSharedMemory);
	}

	if (mod->framebuffer.fbAttached && !msg->attach)
	{
		shmdt(mod->framebuffer.fbSharedMemory);
		mod->framebuffer.fbAttached = FALSE;
		mod->framebuffer.fbSharedMemory = 0;
	}

	return 0;
}

int xrdp_server_reset(xrdpModule* mod, XRDP_MSG_RESET* msg)
{
	xrdpWm* wm;
	rdpSettings* settings;

	wm = (xrdpWm*) (mod->wm);
	settings = wm->session->settings;

	if (libxrdp_reset(wm->session, msg) != 0)
		return 0;

	xrdp_cache_reset(wm->cache);
	xrdp_bitmap_resize(wm->screen, settings->DesktopWidth, settings->DesktopHeight);
	xrdp_wm_load_static_pointers(wm);

	return 0;
}

int xrdp_server_create_offscreen_surface(xrdpModule* mod, XRDP_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	int status;
	xrdpWm* wm;
	xrdpBitmap *bitmap;

	wm = (xrdpWm*) (mod->wm);

	bitmap = xrdp_bitmap_create(msg->nWidth, msg->nHeight, wm->screen->bpp, WND_TYPE_OFFSCREEN, wm);
	status = xrdp_cache_add_offscreen_bitmap(wm->cache, bitmap, msg->cacheIndex);

	if (status != 0)
	{
		log_message(LOG_LEVEL_ERROR, "server_create_os_surface: xrdp_cache_add_os_bitmap failed");
		return 1;
	}

	bitmap->item_index = msg->cacheIndex;
	bitmap->id = msg->cacheIndex;

	return 0;
}

int xrdp_server_switch_offscreen_surface(xrdpModule* mod, XRDP_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	xrdpWm* wm;
	xrdpOffscreenBitmapItem* bi;
	xrdpPainter* p;

	wm = (xrdpWm*) (mod->wm);

	if (msg->cacheIndex == -1)
	{
		wm->target_surface = wm->screen;
		p = (xrdpPainter*) (mod->painter);

		if (p != 0)
		{
			wm_painter_set_target(p);
		}

		return 0;
	}

	bi = xrdp_cache_get_offscreen_bitmap(wm->cache, msg->cacheIndex);

	if (bi != 0)
	{
		wm->target_surface = bi->bitmap;
		p = (xrdpPainter*) (mod->painter);

		if (p != 0)
		{
			wm_painter_set_target(p);
		}
	}
	else
	{
		log_message(LOG_LEVEL_ERROR, "server_switch_os_surface: error finding id %d", msg->cacheIndex);
	}

	return 0;
}

int xrdp_server_delete_offscreen_surface(xrdpModule* mod, XRDP_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	xrdpWm* wm;
	xrdpPainter* p;

	wm = (xrdpWm*) (mod->wm);

	if (wm->target_surface->type == WND_TYPE_OFFSCREEN)
	{
		if (wm->target_surface->id == msg->cacheIndex)
		{
			g_writeln("server_delete_os_surface: setting target_surface to screen");

			wm->target_surface = wm->screen;
			p = (xrdpPainter*) (mod->painter);

			if (p != 0)
			{
				wm_painter_set_target(p);
			}
		}
	}

	xrdp_cache_remove_offscreen_bitmap(wm->cache, msg->cacheIndex);

	return 0;
}

int xrdp_server_paint_offscreen_surface(xrdpModule* mod, XRDP_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	xrdpWm* wm;
	xrdpBitmap* b;
	xrdpPainter* p;
	xrdpOffscreenBitmapItem* bi;

	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	wm = (xrdpWm*) (mod->wm);
	bi = xrdp_cache_get_offscreen_bitmap(wm->cache, msg->cacheIndex);

	if (bi)
	{
		b = bi->bitmap;
		xrdp_painter_copy(p, b, wm->target_surface, msg->nLeftRect, msg->nTopRect,
				msg->nWidth, msg->nHeight, msg->nXSrc, msg->nYSrc);
	}
	else
	{
		log_message(LOG_LEVEL_ERROR, "server_paint_rect_os: error finding id %d", msg->cacheIndex);
	}

	return 0;
}

int xrdp_server_window_new_update(xrdpModule* mod, XRDP_MSG_WINDOW_NEW_UPDATE* msg)
{
	xrdpWm* wm = (xrdpWm*) (mod->wm);
	return libxrdp_window_new_update(wm->session, msg);
}

int xrdp_server_window_delete(xrdpModule* mod, XRDP_MSG_WINDOW_DELETE* msg)
{
	xrdpWm* wm = (xrdpWm*) (mod->wm);
	return libxrdp_window_delete(wm->session, msg);
}

int xrdp_server_window_icon(xrdpModule* mod, int window_id, int cache_entry, int cache_id,
		xrdpRailIconInfo* icon_info, int flags)
{
	xrdpWm* wm = (xrdpWm*) (mod->wm);
	return libxrdp_window_icon(wm->session, window_id, cache_entry, cache_id, icon_info, flags);
}

int xrdp_server_window_cached_icon(xrdpModule* mod, int window_id, int cache_entry, int cache_id, int flags)
{
	xrdpWm* wm = (xrdpWm*) (mod->wm);
	return libxrdp_window_cached_icon(wm->session, window_id, cache_entry, cache_id, flags);
}

int xrdp_server_notify_new_update(xrdpModule* mod, int window_id, int notify_id,
		xrdpRailNotifyStateOrder* notify_state, int flags)
{
	xrdpWm* wm = (xrdpWm*) (mod->wm);
	return libxrdp_notify_new_update(wm->session, window_id, notify_id, notify_state, flags);
}

int xrdp_server_notify_delete(xrdpModule* mod, int window_id, int notify_id)
{
	xrdpWm* wm = (xrdpWm*) (mod->wm);
	return libxrdp_notify_delete(wm->session, window_id, notify_id);
}

int xrdp_server_monitored_desktop(xrdpModule* mod, xrdpRailMonitoredDesktopOrder* mdo, int flags)
{
	xrdpWm* wm = (xrdpWm*) (mod->wm);
	return libxrdp_monitored_desktop(wm->session, mdo, flags);
}

int xrdp_server_module_init(xrdpModule* mod)
{
	mod->server = (xrdpServerModule*) malloc(sizeof(xrdpServerModule));

	if (mod->server)
	{
		ZeroMemory(mod->server, sizeof(xrdpServerModule));

		mod->server->BeginUpdate = xrdp_server_begin_update;
		mod->server->EndUpdate = xrdp_server_end_update;
		mod->server->Beep = xrdp_server_beep;
		mod->server->IsTerminated = xrdp_server_is_terminated;
		mod->server->OpaqueRect = xrdp_server_opaque_rect;
		mod->server->ScreenBlt = xrdp_server_screen_blt;
		mod->server->PaintRect = xrdp_server_paint_rect;
		mod->server->PatBlt = xrdp_server_patblt;
		mod->server->DstBlt = xrdp_server_dstblt;
		mod->server->SetPointer = xrdp_server_set_pointer;
		mod->server->SetPalette = xrdp_server_set_palette;
		mod->server->SetClippingRegion = xrdp_server_set_clipping_region;
		mod->server->LineTo = xrdp_server_line_to;
		mod->server->CacheGlyph = xrdp_server_cache_glyph;
		mod->server->GlyphIndex = xrdp_server_glyph_index;
		mod->server->SharedFramebuffer = xrdp_server_shared_framebuffer;
		mod->server->Reset = xrdp_server_reset;
		mod->server->CreateOffscreenSurface = xrdp_server_create_offscreen_surface;
		mod->server->SwitchOffscreenSurface = xrdp_server_switch_offscreen_surface;
		mod->server->DeleteOffscreenSurface = xrdp_server_delete_offscreen_surface;
		mod->server->PaintOffscreenSurface = xrdp_server_paint_offscreen_surface;
		mod->server->WindowNewUpdate = xrdp_server_window_new_update;
		mod->server->WindowDelete = xrdp_server_window_delete;
		mod->server->WindowIcon = xrdp_server_window_icon;
		mod->server->WindowCachedIcon = xrdp_server_window_cached_icon;
		mod->server->NotifyNewUpdate = xrdp_server_notify_new_update;
		mod->server->NotifyDelete = xrdp_server_notify_delete;
		mod->server->MonitoredDesktop = xrdp_server_monitored_desktop;
	}

	xrdp_message_server_module_init(mod);

	return 0;
}
