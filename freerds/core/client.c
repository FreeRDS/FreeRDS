/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <winpr/crt.h>

#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef _WIN32
#include <sys/shm.h>
#include <sys/stat.h>
#endif

#include "freerds.h"

#include <freerds/backend.h>

void* freerds_client_thread(void* arg)
{
	int fps;
	DWORD status;
	DWORD nCount;
	HANDLE events[8];
	HANDLE PackTimer;
	LARGE_INTEGER due;
	rdsBackendConnector* connector = (rdsBackendConnector*) arg;

	fps = connector->fps;
	PackTimer = CreateWaitableTimer(NULL, TRUE, NULL);

	due.QuadPart = 0;
	SetWaitableTimer(PackTimer, &due, 1000 / fps, NULL, NULL, 0);

	nCount = 0;
	events[nCount++] = PackTimer;
	events[nCount++] = connector->StopEvent;
	events[nCount++] = connector->hClientPipe;

	while (1)
	{
		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		if (WaitForSingleObject(connector->StopEvent, 0) == WAIT_OBJECT_0)
		{
			break;
		}

		if (WaitForSingleObject(connector->hClientPipe, 0) == WAIT_OBJECT_0)
		{
			if (freerds_transport_receive((rdsBackend*) connector) < 0)
				break;
		}

		if (status == WAIT_OBJECT_0)
		{
			freerds_message_server_queue_pack(connector);

			if (connector->fps != fps)
			{
				fps = connector->fps;
				due.QuadPart = 0;
				SetWaitableTimer(PackTimer, &due, 1000 / fps, NULL, NULL, 0);
			}
		}
	}

	CloseHandle(PackTimer);

	return NULL;
}

int freerds_client_get_event_handles(rdsBackend* backend, HANDLE* events, DWORD* nCount)
{
	rdsBackendConnector* connector = (rdsBackendConnector*) backend;

	if (connector)
	{
		if (connector->ServerQueue)
		{
			events[*nCount] = MessageQueue_Event(connector->ServerQueue);
			(*nCount)++;
		}
	}

	return 0;
}

int freerds_client_check_event_handles(rdsBackend* backend)
{
	int status = 0;

	rdsBackendConnector* connector = (rdsBackendConnector*) backend;

	if (!connector)
		return 0;

	while (WaitForSingleObject(MessageQueue_Event(connector->ServerQueue), 0) == WAIT_OBJECT_0)
	{
		status = freerds_message_server_queue_process_pending_messages(connector);
	}

	return status;
}

/* server callbacks */

int freerds_client_inbound_begin_update(rdsBackend* backend, RDS_MSG_BEGIN_UPDATE* msg)
{
	freerds_orders_begin_paint(((rdsBackendConnector*) backend)->connection);
	return 0;
}

int freerds_client_inbound_end_update(rdsBackend* backend, RDS_MSG_END_UPDATE* msg)
{
	freerds_orders_end_paint(((rdsBackendConnector*) backend)->connection);
	backend->client->VBlankEvent(backend);
	return 0;
}

int freerds_client_inbound_beep(rdsBackend* backend, RDS_MSG_BEEP* msg)
{
	freerds_send_bell(((rdsBackendConnector*) backend)->connection);
	return 0;
}

int freerds_client_inbound_is_terminated(rdsBackend* backend)
{
	return g_is_term();
}

int freerds_client_inbound_opaque_rect(rdsBackend* backend, RDS_MSG_OPAQUE_RECT* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_screen_blt(rdsBackend* backend, RDS_MSG_SCREEN_BLT* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_paint_rect(rdsBackend* backend, RDS_MSG_PAINT_RECT* msg)
{
	int bpp;
	int inFlightFrames;
	SURFACE_FRAME* frame;
	rdsConnection* connection;
	rdpSettings* settings;
	rdsBackendConnector* connector = (rdsBackendConnector*) backend;

	connection = connector->connection;
	settings = connection->settings;

	if (!msg->framebuffer->fbAttached)
		return 0;

	if ((msg->nXSrc + msg->nWidth) > msg->framebuffer->fbWidth)
		msg->nWidth = msg->framebuffer->fbWidth - msg->nXSrc;

	if ((msg->nYSrc + msg->nHeight) > msg->framebuffer->fbHeight)
		msg->nHeight = msg->framebuffer->fbHeight - msg->nYSrc;

	if (connection->codecMode)
	{
		bpp = msg->framebuffer->fbBitsPerPixel;

		inFlightFrames = ListDictionary_Count(connection->FrameList);

		if (inFlightFrames > settings->FrameAcknowledge)
		{
			connector->fps = (100 / (inFlightFrames + 1) * connector->MaxFps) / 100;
		}
		else
		{
			connector->fps += 2;

			if (connector->fps > connector->MaxFps)
				connector->fps = connector->MaxFps;
		}

		if (connector->fps < 1)
			connector->fps = 1;

		frame = (SURFACE_FRAME*) malloc(sizeof(SURFACE_FRAME));

		frame->frameId = ++connection->frameId;
		ListDictionary_Add(connection->FrameList, (void*) (size_t) frame->frameId, frame);

		if (settings->SurfaceFrameMarkerEnabled)
			freerds_orders_send_frame_marker(connection, SURFACECMD_FRAMEACTION_BEGIN, frame->frameId);

		freerds_send_surface_bits(connection, bpp, msg);

		if (settings->SurfaceFrameMarkerEnabled)
			freerds_orders_send_frame_marker(connection, SURFACECMD_FRAMEACTION_END, frame->frameId);
	}
	else
	{
		bpp = settings->ColorDepth;
		freerds_send_bitmap_update(connection, bpp, msg);
	}

	return 0;
}

int freerds_client_inbound_patblt(rdsBackend* backend, RDS_MSG_PATBLT* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_dstblt(rdsBackend* backend, RDS_MSG_DSTBLT* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_set_pointer(rdsBackend* backend, RDS_MSG_SET_POINTER* msg)
{
	freerds_set_pointer(((rdsBackendConnector*) backend)->connection, msg);
	return 0;
}

int freerds_client_inbound_set_system_pointer(rdsBackend* backend, RDS_MSG_SET_SYSTEM_POINTER* msg)
{
	freerds_set_system_pointer(((rdsBackendConnector*) backend)->connection, msg);
	return 0;
}

int freerds_client_inbound_set_palette(rdsBackend* backend, RDS_MSG_SET_PALETTE* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_set_clipping_region(rdsBackend* backend, RDS_MSG_SET_CLIPPING_REGION* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_line_to(rdsBackend* backend, RDS_MSG_LINE_TO* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_cache_glyph(rdsBackend* backend, RDS_MSG_CACHE_GLYPH* msg)
{
	return freerds_orders_send_font(((rdsBackendConnector*) backend)->connection, msg);
}

int freerds_client_inbound_glyph_index(rdsBackend* backend, RDS_MSG_GLYPH_INDEX* msg)
{
	/* TODO */

	return 0;
}

static void detach_framebuffer(RDS_FRAMEBUFFER* framebuffer)
{
#ifndef _WIN32
	fprintf(stderr, "detaching segment %d from %p\n",
			framebuffer->fbSegmentId, framebuffer->fbSharedMemory);

	shmdt(framebuffer->fbSharedMemory);
#endif

	ZeroMemory(framebuffer, sizeof(RDS_FRAMEBUFFER));
}

int freerds_client_inbound_shared_framebuffer(rdsBackend* backend, RDS_MSG_SHARED_FRAMEBUFFER* msg)
{
	int attach;
	rdpUpdate* update;
	rdpContext* context;
	rdsBackendConnector* connector;

	connector = (rdsBackendConnector*) backend;
	context = (rdpContext*) connector->connection;
	update = context->update;

	attach = (msg->flags & RDS_FRAMEBUFFER_FLAG_ATTACH) ? TRUE : FALSE;

	fprintf(stderr, "freerds_client_inbound_shared_framebuffer: segmentId: %d width: %d height: %d attach: %d\n",
			msg->segmentId, msg->width, msg->height, attach);

	if (attach)
	{
		void* addr;
		RDS_MSG_PAINT_RECT fm;
		rdpSettings* settings = connector->settings;
		UINT32 DesktopWidth = msg->width;
		UINT32 DesktopHeight = msg->height;

		if (backend->framebuffer.fbAttached)
		{
			detach_framebuffer(&(backend->framebuffer));
		}

		backend->framebuffer.fbWidth = msg->width;
		backend->framebuffer.fbHeight = msg->height;
		backend->framebuffer.fbScanline = msg->scanline;
		backend->framebuffer.fbSegmentId = msg->segmentId;
		backend->framebuffer.fbBitsPerPixel = msg->bitsPerPixel;
		backend->framebuffer.fbBytesPerPixel = msg->bytesPerPixel;

#ifndef _WIN32
		addr = shmat(backend->framebuffer.fbSegmentId, 0, SHM_RDONLY);

		if (addr == ((void*) (size_t) (-1)))
		{
			fprintf(stderr, "failed to attach to segment %d, errno: %d\n",
					backend->framebuffer.fbSegmentId, errno);
			return 1;
		}
#else
		addr = NULL;
#endif

		backend->framebuffer.fbSharedMemory = (BYTE*) addr;
		backend->framebuffer.fbAttached = 1;

		fprintf(stderr, "attached segment %d to %p\n",
				backend->framebuffer.fbSegmentId, backend->framebuffer.fbSharedMemory);

		if ((DesktopWidth != settings->DesktopWidth) || (DesktopHeight != settings->DesktopHeight))
		{
			fprintf(stderr, "Resizing client to %dx%d\n", DesktopWidth, DesktopHeight);

			settings->DesktopWidth = DesktopWidth;
			settings->DesktopHeight = DesktopHeight;

			update->DesktopResize(context);
		}

		fm.type = RDS_SERVER_PAINT_RECT;
		fm.nTopRect = 0;
		fm.nLeftRect = 0;
		fm.nWidth = msg->width;
		fm.nHeight = msg->height;
		fm.fbSegmentId = backend->framebuffer.fbSegmentId;
		fm.bitmapData = NULL;
		fm.bitmapDataLength = 0;
		fm.nXSrc = 0;
		fm.nYSrc = 0;
		fm.framebuffer = &(backend->framebuffer);

		freerds_client_inbound_paint_rect(backend, &fm);
	}
	else
	{
		if (backend->framebuffer.fbAttached)
		{
			detach_framebuffer(&(backend->framebuffer));
		}
	}

	backend->client->VBlankEvent(backend);

	return 0;
}

int freerds_client_inbound_reset(rdsBackend* backend, RDS_MSG_RESET* msg)
{
	if (freerds_reset(((rdsBackendConnector*) backend)->connection, msg) != 0)
		return 0;

	return 0;
}

int freerds_client_inbound_create_offscreen_surface(rdsBackend* backend, RDS_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int freerds_client_inbound_switch_offscreen_surface(rdsBackend* backend, RDS_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int freerds_client_inbound_delete_offscreen_surface(rdsBackend* backend, RDS_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int freerds_client_inbound_paint_offscreen_surface(rdsBackend* backend, RDS_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int freerds_client_inbound_window_new_update(rdsBackend* backend, RDS_MSG_WINDOW_NEW_UPDATE* msg)
{
	return freerds_window_new_update(((rdsBackendConnector*) backend)->connection, msg);
}

int freerds_client_inbound_window_delete(rdsBackend* backend, RDS_MSG_WINDOW_DELETE* msg)
{
	return freerds_window_delete(((rdsBackendConnector*) backend)->connection, msg);
}

int freerds_client_inbound_logon_user(rdsBackend* backend, RDS_MSG_LOGON_USER* msg)
{

	return 0;
}

int freerds_client_inbound_logoff_user(rdsBackend* backend, RDS_MSG_LOGOFF_USER* msg)
{
	return 0;
}

int freerds_client_inbound_connector_init(rdsBackendConnector* connector)
{
	rdsServerInterface* serverInter;

	if (connector->server)
	{
		serverInter = connector->server;
		serverInter->BeginUpdate = freerds_client_inbound_begin_update;
		serverInter->EndUpdate = freerds_client_inbound_end_update;
		serverInter->Beep = freerds_client_inbound_beep;
		serverInter->IsTerminated = freerds_client_inbound_is_terminated;
		serverInter->OpaqueRect = freerds_client_inbound_opaque_rect;
		serverInter->ScreenBlt = freerds_client_inbound_screen_blt;
		serverInter->PaintRect = freerds_client_inbound_paint_rect;
		serverInter->PatBlt = freerds_client_inbound_patblt;
		serverInter->DstBlt = freerds_client_inbound_dstblt;
		serverInter->SetPointer = freerds_client_inbound_set_pointer;
		serverInter->SetSystemPointer = freerds_client_inbound_set_system_pointer;
		serverInter->SetPalette = freerds_client_inbound_set_palette;
		serverInter->SetClippingRegion = freerds_client_inbound_set_clipping_region;
		serverInter->LineTo = freerds_client_inbound_line_to;
		serverInter->CacheGlyph = freerds_client_inbound_cache_glyph;
		serverInter->GlyphIndex = freerds_client_inbound_glyph_index;
		serverInter->SharedFramebuffer = freerds_client_inbound_shared_framebuffer;
		serverInter->Reset = freerds_client_inbound_reset;
		serverInter->CreateOffscreenSurface = freerds_client_inbound_create_offscreen_surface;
		serverInter->SwitchOffscreenSurface = freerds_client_inbound_switch_offscreen_surface;
		serverInter->DeleteOffscreenSurface = freerds_client_inbound_delete_offscreen_surface;
		serverInter->PaintOffscreenSurface = freerds_client_inbound_paint_offscreen_surface;
		serverInter->WindowNewUpdate = freerds_client_inbound_window_new_update;
		serverInter->WindowDelete = freerds_client_inbound_window_delete;
		serverInter->LogonUser = freerds_client_inbound_logon_user;
		serverInter->LogoffUser = freerds_client_inbound_logoff_user;
	}

	freerds_message_server_connector_init(connector);

	return 0;
}
