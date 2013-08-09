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

#include "xrdp.h"

int xrdp_server_begin_update(xrdpModule* mod, XRDP_MSG_BEGIN_UPDATE* msg)
{
	libxrdp_orders_init(mod->session);
	return 0;
}

int xrdp_server_end_update(xrdpModule* mod, XRDP_MSG_END_UPDATE* msg)
{
	libxrdp_orders_send(mod->session);
	return 0;
}

int xrdp_server_beep(xrdpModule* mod, XRDP_MSG_BEEP* msg)
{
	libxrdp_send_bell(mod->session);
	return 0;
}

int xrdp_server_is_terminated(xrdpModule* mod)
{
	return g_is_term();
}

int xrdp_server_opaque_rect(xrdpModule* mod, XRDP_MSG_OPAQUE_RECT* msg)
{
	/* TODO */

	return 0;
}

int xrdp_server_screen_blt(xrdpModule* mod, XRDP_MSG_SCREEN_BLT* msg)
{
	/* TODO */

	return 0;
}

int xrdp_server_paint_rect(xrdpModule* mod, XRDP_MSG_PAINT_RECT* msg)
{
	int bpp;

	bpp = msg->framebuffer->fbBitsPerPixel;

	if (mod->session->codecMode)
	{
		libxrdp_send_surface_bits(mod->session, bpp, msg);
	}
	else
	{
		libxrdp_send_bitmap_update(mod->session, bpp, msg);
	}

	return 0;
}

int xrdp_server_patblt(xrdpModule* mod, XRDP_MSG_PATBLT* msg)
{
	/* TODO */

	return 0;
}

int xrdp_server_dstblt(xrdpModule* mod, XRDP_MSG_DSTBLT* msg)
{
	/* TODO */

	return 0;
}

int xrdp_server_set_pointer(xrdpModule* mod, XRDP_MSG_SET_POINTER* msg)
{
	/* TODO */

	return 0;
}

int xrdp_server_set_palette(xrdpModule* mod, XRDP_MSG_SET_PALETTE* msg)
{
	/* TODO */

	return 0;
}

int xrdp_server_set_clipping_region(xrdpModule* mod, XRDP_MSG_SET_CLIPPING_REGION* msg)
{
	/* TODO */

	return 0;
}

int xrdp_server_line_to(xrdpModule* mod, XRDP_MSG_LINE_TO* msg)
{
	/* TODO */

	return 0;
}

int xrdp_server_cache_glyph(xrdpModule* mod, XRDP_MSG_CACHE_GLYPH* msg)
{
	return libxrdp_orders_send_font(mod->session, msg);
}

int xrdp_server_glyph_index(xrdpModule* mod, XRDP_MSG_GLYPH_INDEX* msg)
{
	/* TODO */

	return 0;
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

		mod->framebuffer.image = (void*) pixman_image_create_bits(PIXMAN_x8r8g8b8,
				mod->framebuffer.fbWidth, mod->framebuffer.fbHeight,
				(uint32_t*) mod->framebuffer.fbSharedMemory, mod->framebuffer.fbScanline);
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
	if (libxrdp_reset(mod->session, msg) != 0)
		return 0;

	return 0;
}

int xrdp_server_create_offscreen_surface(xrdpModule* mod, XRDP_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int xrdp_server_switch_offscreen_surface(xrdpModule* mod, XRDP_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int xrdp_server_delete_offscreen_surface(xrdpModule* mod, XRDP_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int xrdp_server_paint_offscreen_surface(xrdpModule* mod, XRDP_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int xrdp_server_window_new_update(xrdpModule* mod, XRDP_MSG_WINDOW_NEW_UPDATE* msg)
{
	return libxrdp_window_new_update(mod->session, msg);
}

int xrdp_server_window_delete(xrdpModule* mod, XRDP_MSG_WINDOW_DELETE* msg)
{
	return libxrdp_window_delete(mod->session, msg);
}

int xrdp_server_window_icon(xrdpModule* mod, int window_id, int cache_entry, int cache_id,
		xrdpRailIconInfo* icon_info, int flags)
{
	return libxrdp_window_icon(mod->session, window_id, cache_entry, cache_id, icon_info, flags);
}

int xrdp_server_window_cached_icon(xrdpModule* mod, int window_id, int cache_entry, int cache_id, int flags)
{
	return libxrdp_window_cached_icon(mod->session, window_id, cache_entry, cache_id, flags);
}

int xrdp_server_notify_new_update(xrdpModule* mod, int window_id, int notify_id,
		xrdpRailNotifyStateOrder* notify_state, int flags)
{
	return libxrdp_notify_new_update(mod->session, window_id, notify_id, notify_state, flags);
}

int xrdp_server_notify_delete(xrdpModule* mod, int window_id, int notify_id)
{
	return libxrdp_notify_delete(mod->session, window_id, notify_id);
}

int xrdp_server_monitored_desktop(xrdpModule* mod, xrdpRailMonitoredDesktopOrder* mdo, int flags)
{
	return libxrdp_monitored_desktop(mod->session, mdo, flags);
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
