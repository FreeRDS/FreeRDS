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

#include "log.h"

#include "xrdp.h"

int xrdp_server_begin_update(xrdpModule* mod)
{
	xrdpWm* wm;
	xrdpPainter* p;

	wm = (xrdpWm*) (mod->wm);
	p = xrdp_painter_create(wm, wm->session);
	xrdp_painter_begin_update(p);
	mod->painter = (long) p;

	return 0;
}

int xrdp_server_end_update(xrdpModule* mod)
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

int xrdp_server_beep(xrdpModule* mod)
{
	xrdpWm* wm;

	wm = (xrdpWm*) (mod->wm);
	xrdp_wm_send_bell(wm);

	return 0;
}

int xrdp_server_message(xrdpModule* mod, char *msg, int code)
{
	xrdpWm* wm;

	if (code == 1)
	{
		g_writeln(msg);
		return 0;
	}

	wm = (xrdpWm*) (mod->wm);

	return xrdp_wm_log_msg(wm, msg);
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
	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

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

int xrdp_server_set_pointer(xrdpModule* mod, XRDP_MSG_SET_POINTER* msg)
{
	xrdpWm* wm = (xrdpWm*) (mod->wm);

	if (msg->xorBpp == 1)
		msg->xorBpp = 0;

	xrdp_wm_pointer(wm, (char*) msg->xorMaskData, (char*) msg->andMaskData, msg->xPos, msg->yPos, msg->xorBpp);

	return 0;
}

int xrdp_server_set_palette(xrdpModule* mod, int* palette)
{
	xrdpWm* wm;

	wm = (xrdpWm*) (mod->wm);

	if (g_memcmp(wm->palette, palette, 255 * sizeof(int)) != 0)
	{
		g_memcpy(wm->palette, palette, 256 * sizeof(int));
		xrdp_wm_send_palette(wm);
	}

	return 0;
}

int xrdp_server_set_clipping_region(xrdpModule* mod, XRDP_MSG_SET_CLIPPING_REGION* msg)
{
	xrdpPainter* p;

	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	return xrdp_painter_set_clip(p, msg->nLeftRect, msg->nTopRect, msg->nWidth, msg->nHeight);
}

int xrdp_server_set_null_clipping_region(xrdpModule* mod)
{
	xrdpPainter* p;

	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	return xrdp_painter_clr_clip(p);
}

int xrdp_server_set_forecolor(xrdpModule* mod, int fgcolor)
{
	xrdpPainter* p;

	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	p->fg_color = fgcolor;
	p->pen.color = p->fg_color;

	return 0;
}

int xrdp_server_set_backcolor(xrdpModule* mod, int bgcolor)
{
	xrdpPainter* p;

	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	p->bg_color = bgcolor;

	return 0;
}

int xrdp_server_set_rop2(xrdpModule* mod, int opcode)
{
	xrdpPainter* p;

	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	p->rop = opcode;

	return 0;
}

int xrdp_server_line_to(xrdpModule* mod, XRDP_MSG_LINE_TO* msg)
{
	xrdpWm* wm;
	xrdpPainter* p;

	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	wm = (xrdpWm*) (mod->wm);

	return xrdp_painter_line(p, wm->target_surface, msg->nXStart, msg->nYStart, msg->nXEnd, msg->nYEnd);
}

int xrdp_server_add_char(xrdpModule* mod, XRDP_MSG_CACHE_GLYPH* msg)
{
	return libxrdp_orders_send_font(((xrdpWm*) mod->wm)->session, msg);
}

int xrdp_server_text(xrdpModule* mod, GLYPH_INDEX_ORDER* msg)
{
	xrdpWm* wm;
	xrdpPainter* p;

	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	wm = (xrdpWm*) (mod->wm);

	return xrdp_painter_draw_text2(p, wm->target_surface, msg);
}

int xrdp_server_reset(xrdpModule* mod, int width, int height, int bpp)
{
	xrdpWm* wm;
	rdpSettings* settings;

	wm = (xrdpWm*) (mod->wm);
	settings = wm->session->settings;

	/* older client can't resize */
	if (settings->ClientBuild <= 419)
	{
		return 0;
	}

	/* if same, don't need to do anything */
	if ((settings->DesktopWidth == width) && (settings->DesktopHeight == height) && (settings->ColorDepth == bpp))
	{
		return 0;
	}

	if (libxrdp_reset(wm->session, width, height, bpp) != 0)
	{
		return 1;
	}

	/* reset cache */
	xrdp_cache_reset(wm->cache);
	/* resize the main window */
	xrdp_bitmap_resize(wm->screen, settings->DesktopWidth, settings->DesktopHeight);
	/* load some stuff */
	xrdp_wm_load_static_colors_plus(wm, 0);
	xrdp_wm_load_static_pointers(wm);

	return 0;
}

int xrdp_server_create_offscreen_surface(xrdpModule* mod, int rdpindex, int width, int height)
{
	xrdpWm* wm;
	xrdpBitmap *bitmap;
	int error;

	wm = (xrdpWm*) (mod->wm);
	bitmap = xrdp_bitmap_create(width, height, wm->screen->bpp, WND_TYPE_OFFSCREEN, wm);
	error = xrdp_cache_add_os_bitmap(wm->cache, bitmap, rdpindex);

	if (error != 0)
	{
		log_message(LOG_LEVEL_ERROR, "server_create_os_surface: xrdp_cache_add_os_bitmap failed");
		return 1;
	}

	bitmap->item_index = rdpindex;
	bitmap->id = rdpindex;

	return 0;
}

int xrdp_server_switch_offscreen_surface(xrdpModule* mod, int rdpindex)
{
	xrdpWm* wm;
	xrdpOffscreenBitmapItem *bi;
	xrdpPainter* p;

	wm = (xrdpWm*) (mod->wm);

	if (rdpindex == -1)
	{
		wm->target_surface = wm->screen;
		p = (xrdpPainter*) (mod->painter);

		if (p != 0)
		{
			wm_painter_set_target(p);
		}

		return 0;
	}

	bi = xrdp_cache_get_os_bitmap(wm->cache, rdpindex);

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
		log_message(LOG_LEVEL_ERROR, "server_switch_os_surface: error finding id %d", rdpindex);
	}

	return 0;
}

int xrdp_server_delete_offscreen_surface(xrdpModule* mod, int rdpindex)
{
	xrdpWm* wm;
	xrdpPainter* p;

	wm = (xrdpWm*) (mod->wm);

	if (wm->target_surface->type == WND_TYPE_OFFSCREEN)
	{
		if (wm->target_surface->id == rdpindex)
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

	xrdp_cache_remove_os_bitmap(wm->cache, rdpindex);

	return 0;
}

int xrdp_server_paint_offscreen_rect(xrdpModule* mod, int x, int y, int cx, int cy, int rdpindex, int srcx, int srcy)
{
	xrdpWm* wm;
	xrdpBitmap *b;
	xrdpPainter* p;
	xrdpOffscreenBitmapItem *bi;

	p = (xrdpPainter*) (mod->painter);

	if (!p)
		return 0;

	wm = (xrdpWm*) (mod->wm);
	bi = xrdp_cache_get_os_bitmap(wm->cache, rdpindex);

	if (bi != 0)
	{
		b = bi->bitmap;
		xrdp_painter_copy(p, b, wm->target_surface, x, y, cx, cy, srcx, srcy);
	}
	else
	{
		log_message(LOG_LEVEL_ERROR, "server_paint_rect_os: error finding id %d", rdpindex);
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
		mod->server->Message = xrdp_server_message;
		mod->server->IsTerminated = xrdp_server_is_terminated;
		mod->server->OpaqueRect = xrdp_server_opaque_rect;
		mod->server->ScreenBlt = xrdp_server_screen_blt;
		mod->server->PaintRect = xrdp_server_paint_rect;
		mod->server->PatBlt = xrdp_server_patblt;
		mod->server->SetPointer = xrdp_server_set_pointer;
		mod->server->SetPalette = xrdp_server_set_palette;
		mod->server->SetClippingRegion = xrdp_server_set_clipping_region;
		mod->server->SetNullClippingRegion = xrdp_server_set_null_clipping_region;
		mod->server->SetForeColor = xrdp_server_set_forecolor;
		mod->server->SetBackColor = xrdp_server_set_backcolor;
		mod->server->SetRop2 = xrdp_server_set_rop2;
		mod->server->LineTo = xrdp_server_line_to;
		mod->server->AddChar = xrdp_server_add_char;
		mod->server->Text = xrdp_server_text;
		mod->server->Reset = xrdp_server_reset;
		mod->server->CreateOffscreenSurface = xrdp_server_create_offscreen_surface;
		mod->server->SwitchOffscreenSurface = xrdp_server_switch_offscreen_surface;
		mod->server->DeleteOffscreenSurface = xrdp_server_delete_offscreen_surface;
		mod->server->PaintOffscreenRect = xrdp_server_paint_offscreen_rect;
		mod->server->WindowNewUpdate = xrdp_server_window_new_update;
		mod->server->WindowDelete = xrdp_server_window_delete;
		mod->server->WindowIcon = xrdp_server_window_icon;
		mod->server->WindowCachedIcon = xrdp_server_window_cached_icon;
		mod->server->NotifyNewUpdate = xrdp_server_notify_new_update;
		mod->server->NotifyDelete = xrdp_server_notify_delete;
		mod->server->MonitoredDesktop = xrdp_server_monitored_desktop;
	}

	return 0;
}
