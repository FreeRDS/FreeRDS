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

int freerds_client_inbound_begin_update(rdsModule* module, RDS_MSG_BEGIN_UPDATE* msg)
{
	rdsConnector* connector = (rdsConnector*) module;
	libxrdp_orders_begin_paint(connector->session);
	return 0;
}

int freerds_client_inbound_end_update(rdsModule* module, RDS_MSG_END_UPDATE* msg)
{
	rdsConnector* connector = (rdsConnector*) module;
	libxrdp_orders_end_paint(connector->session);
	return 0;
}

int freerds_client_inbound_beep(rdsModule* module, RDS_MSG_BEEP* msg)
{
	rdsConnector* connector = (rdsConnector*) module;
	libxrdp_send_bell(connector->session);
	return 0;
}

int freerds_client_inbound_is_terminated(rdsModule* module)
{
	return g_is_term();
}

int freerds_client_inbound_opaque_rect(rdsModule* module, RDS_MSG_OPAQUE_RECT* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_screen_blt(rdsModule* module, RDS_MSG_SCREEN_BLT* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_paint_rect(rdsModule* module, RDS_MSG_PAINT_RECT* msg)
{
	int bpp;
	int inFlightFrames;
	SURFACE_FRAME* frame;
	rdsSession* session;
	rdpSettings* settings;
	rdsConnector* connector = (rdsConnector*) module;

	session = connector->session;
	settings = session->settings;

	bpp = msg->framebuffer->fbBitsPerPixel;

	if (session->codecMode)
	{
		inFlightFrames = ListDictionary_Count(session->FrameList);

		if (inFlightFrames > settings->FrameAcknowledge)
			connector->fps = (100 / (inFlightFrames + 1) * connector->MaxFps) / 100;
		else
			connector->fps = connector->MaxFps;

		if (connector->fps < 1)
			connector->fps = 1;

		frame = (SURFACE_FRAME*) malloc(sizeof(SURFACE_FRAME));

		frame->frameId = ++session->frameId;
		ListDictionary_Add(session->FrameList, (void*) (size_t) frame->frameId, frame);

		libxrdp_orders_send_frame_marker(session, SURFACECMD_FRAMEACTION_BEGIN, frame->frameId);
		libxrdp_send_surface_bits(session, bpp, msg);
		libxrdp_orders_send_frame_marker(session, SURFACECMD_FRAMEACTION_END, frame->frameId);
	}
	else
	{
		libxrdp_send_bitmap_update(session, bpp, msg);
	}

	return 0;
}

int freerds_client_inbound_patblt(rdsModule* module, RDS_MSG_PATBLT* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_dstblt(rdsModule* module, RDS_MSG_DSTBLT* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_set_pointer(rdsModule* module, RDS_MSG_SET_POINTER* msg)
{
	rdsConnector* connector = (rdsConnector*) module;
	libxrdp_set_pointer(connector->session, msg);
	return 0;
}

int freerds_client_inbound_set_system_pointer(rdsModule* module, RDS_MSG_SET_SYSTEM_POINTER* msg)
{
	rdsConnector* connector = (rdsConnector*) module;
	libxrdp_set_system_pointer(connector->session, msg);
	return 0;
}

int freerds_client_inbound_set_palette(rdsModule* module, RDS_MSG_SET_PALETTE* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_set_clipping_region(rdsModule* module, RDS_MSG_SET_CLIPPING_REGION* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_line_to(rdsModule* module, RDS_MSG_LINE_TO* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_cache_glyph(rdsModule* module, RDS_MSG_CACHE_GLYPH* msg)
{
	rdsConnector* connector = (rdsConnector*) module;
	return libxrdp_orders_send_font(connector->session, msg);
}

int freerds_client_inbound_glyph_index(rdsModule* module, RDS_MSG_GLYPH_INDEX* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_shared_framebuffer(rdsModule* module, RDS_MSG_SHARED_FRAMEBUFFER* msg)
{
	rdsConnector* connector = (rdsConnector*) module;

	connector->framebuffer.fbWidth = msg->width;
	connector->framebuffer.fbHeight = msg->height;
	connector->framebuffer.fbScanline = msg->scanline;
	connector->framebuffer.fbSegmentId = msg->segmentId;
	connector->framebuffer.fbBitsPerPixel = msg->bitsPerPixel;
	connector->framebuffer.fbBytesPerPixel = msg->bytesPerPixel;

	printf("received shared framebuffer message: mod->framebuffer.fbAttached: %d msg->attach: %d\n",
			connector->framebuffer.fbAttached, msg->attach);

	if (!connector->framebuffer.fbAttached && msg->attach)
	{
		connector->framebuffer.fbSharedMemory = (BYTE*) shmat(connector->framebuffer.fbSegmentId, 0, 0);
		connector->framebuffer.fbAttached = TRUE;

		printf("attached segment %d to %p\n",
				connector->framebuffer.fbSegmentId, connector->framebuffer.fbSharedMemory);

		connector->framebuffer.image = (void*) pixman_image_create_bits(PIXMAN_x8r8g8b8,
				connector->framebuffer.fbWidth, connector->framebuffer.fbHeight,
				(uint32_t*) connector->framebuffer.fbSharedMemory, connector->framebuffer.fbScanline);
	}

	if (connector->framebuffer.fbAttached && !msg->attach)
	{
		shmdt(connector->framebuffer.fbSharedMemory);
		connector->framebuffer.fbAttached = FALSE;
		connector->framebuffer.fbSharedMemory = 0;
	}

	return 0;
}

int freerds_client_inbound_reset(rdsModule* module, RDS_MSG_RESET* msg)
{
	rdsConnector* connector = (rdsConnector*) module;

	if (libxrdp_reset(connector->session, msg) != 0)
		return 0;

	return 0;
}

int freerds_client_inbound_create_offscreen_surface(rdsModule* module, RDS_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int freerds_client_inbound_switch_offscreen_surface(rdsModule* module, RDS_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int freerds_client_inbound_delete_offscreen_surface(rdsModule* module, RDS_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int freerds_client_inbound_paint_offscreen_surface(rdsModule* module, RDS_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int freerds_client_inbound_window_new_update(rdsModule* module, RDS_MSG_WINDOW_NEW_UPDATE* msg)
{
	rdsConnector* connector = (rdsConnector*) module;
	return libxrdp_window_new_update(connector->session, msg);
}

int freerds_client_inbound_window_delete(rdsModule* module, RDS_MSG_WINDOW_DELETE* msg)
{
	rdsConnector* connector = (rdsConnector*) module;
	return libxrdp_window_delete(connector->session, msg);
}

int freerds_client_inbound_module_init(rdsModule* module)
{
	if (module->server)
	{
		module->server->BeginUpdate = freerds_client_inbound_begin_update;
		module->server->EndUpdate = freerds_client_inbound_end_update;
		module->server->Beep = freerds_client_inbound_beep;
		module->server->IsTerminated = freerds_client_inbound_is_terminated;
		module->server->OpaqueRect = freerds_client_inbound_opaque_rect;
		module->server->ScreenBlt = freerds_client_inbound_screen_blt;
		module->server->PaintRect = freerds_client_inbound_paint_rect;
		module->server->PatBlt = freerds_client_inbound_patblt;
		module->server->DstBlt = freerds_client_inbound_dstblt;
		module->server->SetPointer = freerds_client_inbound_set_pointer;
		module->server->SetSystemPointer = freerds_client_inbound_set_system_pointer;
		module->server->SetPalette = freerds_client_inbound_set_palette;
		module->server->SetClippingRegion = freerds_client_inbound_set_clipping_region;
		module->server->LineTo = freerds_client_inbound_line_to;
		module->server->CacheGlyph = freerds_client_inbound_cache_glyph;
		module->server->GlyphIndex = freerds_client_inbound_glyph_index;
		module->server->SharedFramebuffer = freerds_client_inbound_shared_framebuffer;
		module->server->Reset = freerds_client_inbound_reset;
		module->server->CreateOffscreenSurface = freerds_client_inbound_create_offscreen_surface;
		module->server->SwitchOffscreenSurface = freerds_client_inbound_switch_offscreen_surface;
		module->server->DeleteOffscreenSurface = freerds_client_inbound_delete_offscreen_surface;
		module->server->PaintOffscreenSurface = freerds_client_inbound_paint_offscreen_surface;
		module->server->WindowNewUpdate = freerds_client_inbound_window_new_update;
		module->server->WindowDelete = freerds_client_inbound_window_delete;
	}

	xrdp_message_server_module_init(module);

	return 0;
}
