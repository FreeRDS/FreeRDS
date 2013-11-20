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

#include "freerds.h"

#include <freerds/auth.h>
#include <freerds/freerds.h>
#include <freerds/module_connector.h>
#include <freerds/icp_client_stubs.h>

int freerds_client_inbound_begin_update(rdsModuleConnector* connector, RDS_MSG_BEGIN_UPDATE* msg)
{
	freerds_orders_begin_paint(connector->connection);
	return 0;
}

int freerds_client_inbound_end_update(rdsModuleConnector* connector, RDS_MSG_END_UPDATE* msg)
{
	freerds_orders_end_paint(connector->connection);
	connector->client->VBlankEvent(connector);
	return 0;
}

int freerds_client_inbound_beep(rdsModuleConnector* connector, RDS_MSG_BEEP* msg)
{
	freerds_send_bell(connector->connection);
	return 0;
}

int freerds_client_inbound_is_terminated(rdsModuleConnector* connector)
{
	return g_is_term();
}

int freerds_client_inbound_opaque_rect(rdsModuleConnector* connector, RDS_MSG_OPAQUE_RECT* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_screen_blt(rdsModuleConnector* connector, RDS_MSG_SCREEN_BLT* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_paint_rect(rdsModuleConnector* connector, RDS_MSG_PAINT_RECT* msg)
{
	int bpp;
	int inFlightFrames;
	SURFACE_FRAME* frame;
	rdsConnection* connection;
	rdpSettings* settings;

	connection = connector->connection;
	settings = connection->settings;

	bpp = msg->framebuffer->fbBitsPerPixel;

	if (connection->codecMode)
	{
		inFlightFrames = ListDictionary_Count(connection->FrameList);

		if (inFlightFrames > settings->FrameAcknowledge)
			connector->fps = (100 / (inFlightFrames + 1) * connector->MaxFps) / 100;
		else
			connector->fps = connector->MaxFps;

		if (connector->fps < 1)
			connector->fps = 1;

		frame = (SURFACE_FRAME*) malloc(sizeof(SURFACE_FRAME));

		frame->frameId = ++connection->frameId;
		ListDictionary_Add(connection->FrameList, (void*) (size_t) frame->frameId, frame);

		freerds_orders_send_frame_marker(connection, SURFACECMD_FRAMEACTION_BEGIN, frame->frameId);
		freerds_send_surface_bits(connection, bpp, msg);
		freerds_orders_send_frame_marker(connection, SURFACECMD_FRAMEACTION_END, frame->frameId);
	}
	else
	{
		freerds_send_bitmap_update(connection, bpp, msg);
	}

	return 0;
}

int freerds_client_inbound_patblt(rdsModuleConnector* connector, RDS_MSG_PATBLT* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_dstblt(rdsModuleConnector* connector, RDS_MSG_DSTBLT* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_set_pointer(rdsModuleConnector* connector, RDS_MSG_SET_POINTER* msg)
{
	freerds_set_pointer(connector->connection, msg);
	return 0;
}

int freerds_client_inbound_set_system_pointer(rdsModuleConnector* connector, RDS_MSG_SET_SYSTEM_POINTER* msg)
{
	freerds_set_system_pointer(connector->connection, msg);
	return 0;
}

int freerds_client_inbound_set_palette(rdsModuleConnector* connector, RDS_MSG_SET_PALETTE* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_set_clipping_region(rdsModuleConnector* connector, RDS_MSG_SET_CLIPPING_REGION* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_line_to(rdsModuleConnector* connector, RDS_MSG_LINE_TO* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_cache_glyph(rdsModuleConnector* connector, RDS_MSG_CACHE_GLYPH* msg)
{
	return freerds_orders_send_font(connector->connection, msg);
}

int freerds_client_inbound_glyph_index(rdsModuleConnector* connector, RDS_MSG_GLYPH_INDEX* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_shared_framebuffer(rdsModuleConnector* connector, RDS_MSG_SHARED_FRAMEBUFFER* msg)
{
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
		RDS_MSG_PAINT_RECT fm;
		connector->framebuffer.fbSharedMemory = (BYTE*) shmat(connector->framebuffer.fbSegmentId, 0, 0);
		connector->framebuffer.fbAttached = TRUE;

		printf("attached segment %d to %p\n",
				connector->framebuffer.fbSegmentId, connector->framebuffer.fbSharedMemory);

		connector->framebuffer.image = (void*) pixman_image_create_bits(PIXMAN_x8r8g8b8,
				connector->framebuffer.fbWidth, connector->framebuffer.fbHeight,
				(uint32_t*) connector->framebuffer.fbSharedMemory, connector->framebuffer.fbScanline);

		fm.nTopRect = 0;
		fm.nLeftRect = 0;
		fm.nWidth = msg->width;
		fm.nHeight = msg->height;
		fm.framebuffer = &(connector->framebuffer);
		freerds_client_inbound_paint_rect(connector, &fm);
	}

	if (connector->framebuffer.fbAttached && !msg->attach)
	{
		shmdt(connector->framebuffer.fbSharedMemory);
		connector->framebuffer.fbAttached = FALSE;
		connector->framebuffer.fbSharedMemory = 0;
	}

	connector->client->VBlankEvent(connector);

	return 0;
}

int freerds_client_inbound_reset(rdsModuleConnector* connector, RDS_MSG_RESET* msg)
{
	if (freerds_reset(connector->connection, msg) != 0)
		return 0;

	return 0;
}

int freerds_client_inbound_create_offscreen_surface(rdsModuleConnector* connector, RDS_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int freerds_client_inbound_switch_offscreen_surface(rdsModuleConnector* connector, RDS_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int freerds_client_inbound_delete_offscreen_surface(rdsModuleConnector* connector, RDS_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int freerds_client_inbound_paint_offscreen_surface(rdsModuleConnector* connector, RDS_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	return 0;
}

int freerds_client_inbound_window_new_update(rdsModuleConnector* connector, RDS_MSG_WINDOW_NEW_UPDATE* msg)
{
	return freerds_window_new_update(connector->connection, msg);
}

int freerds_client_inbound_window_delete(rdsModuleConnector* connector, RDS_MSG_WINDOW_DELETE* msg)
{
	return freerds_window_delete(connector->connection, msg);
}

int freerds_client_inbound_logon_user(rdsModuleConnector* module, RDS_MSG_LOGON_USER* msg)
{
	int icpStatus;
	int authStatus;
	char* endPoint;
	DWORD sessionId;
	HANDLE hClientPipe;
	rdsConnection* connection = module->connection;

	if (msg->Domain)
		fprintf(stderr, "LogonUser: %s\\%s | %s", msg->Domain, msg->User, msg->Password);
	else
		fprintf(stderr, "LogonUser: %s | %s", msg->User, msg->Password);

	authStatus = 0;
	endPoint = NULL;
	sessionId = module->SessionId;

	icpStatus = freerds_icp_LogonUser((UINT32*) &sessionId,
			msg->User, msg->Domain, msg->Password, &authStatus, &endPoint);

	if (icpStatus != 0)
	{
		printf("freerds_icp_LogonUser failed %d\n", icpStatus);
		return -1;
	}

	fprintf(stderr, "Logon Status: %d\n", authStatus);

	connection->connector = freerds_module_connector_new(connection);

	connection->connector->Endpoint = endPoint;
	connection->connector->SessionId = sessionId;

	hClientPipe = freerds_named_pipe_connect(connection->connector->Endpoint, 20);

	if (hClientPipe == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Failed to create named pipe %s\n", connection->connector->Endpoint);
		return FALSE;
	}

	printf("Connected to session %d\n", (int) connection->connector->SessionId);

	if (freerds_init_client(hClientPipe, connection->settings, connection->connector->OutboundStream) < 0)
	{
		fprintf(stderr, "Error sending initial packet with %s\n", connection->connector->Endpoint);
		return FALSE;
	}

	connection->connector->hClientPipe = hClientPipe;
	connection->connector->GetEventHandles = freerds_client_get_event_handles;
	connection->connector->CheckEventHandles = freerds_client_check_event_handles;

	connection->connector->ServerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) freerds_client_thread,
			(void*) connection->connector, CREATE_SUSPENDED, NULL);

	freerds_client_inbound_connector_init(connection->connector);

	ResumeThread(connection->connector->ServerThread);

	if (authStatus != 0)
	{
		RDS_MSG_LOGON_USER logonUser;

		ZeroMemory(&logonUser, sizeof(RDS_MSG_LOGON_USER));
		logonUser.type = RDS_CLIENT_LOGON_USER;

		logonUser.Flags = 0;
		logonUser.User = msg->User;
		logonUser.Domain = msg->Domain;
		logonUser.Password = msg->Password;

		connection->connector->client->LogonUser(connection->connector, &logonUser);
	}

	return 0;
}

int freerds_client_inbound_logoff_user(rdsModuleConnector* module, RDS_MSG_LOGOFF_USER* msg)
{
	return 0;
}

int freerds_client_inbound_connector_init(rdsModuleConnector* connector)
{
	if (connector->server)
	{
		connector->server->BeginUpdate = freerds_client_inbound_begin_update;
		connector->server->EndUpdate = freerds_client_inbound_end_update;
		connector->server->Beep = freerds_client_inbound_beep;
		connector->server->IsTerminated = freerds_client_inbound_is_terminated;
		connector->server->OpaqueRect = freerds_client_inbound_opaque_rect;
		connector->server->ScreenBlt = freerds_client_inbound_screen_blt;
		connector->server->PaintRect = freerds_client_inbound_paint_rect;
		connector->server->PatBlt = freerds_client_inbound_patblt;
		connector->server->DstBlt = freerds_client_inbound_dstblt;
		connector->server->SetPointer = freerds_client_inbound_set_pointer;
		connector->server->SetSystemPointer = freerds_client_inbound_set_system_pointer;
		connector->server->SetPalette = freerds_client_inbound_set_palette;
		connector->server->SetClippingRegion = freerds_client_inbound_set_clipping_region;
		connector->server->LineTo = freerds_client_inbound_line_to;
		connector->server->CacheGlyph = freerds_client_inbound_cache_glyph;
		connector->server->GlyphIndex = freerds_client_inbound_glyph_index;
		connector->server->SharedFramebuffer = freerds_client_inbound_shared_framebuffer;
		connector->server->Reset = freerds_client_inbound_reset;
		connector->server->CreateOffscreenSurface = freerds_client_inbound_create_offscreen_surface;
		connector->server->SwitchOffscreenSurface = freerds_client_inbound_switch_offscreen_surface;
		connector->server->DeleteOffscreenSurface = freerds_client_inbound_delete_offscreen_surface;
		connector->server->PaintOffscreenSurface = freerds_client_inbound_paint_offscreen_surface;
		connector->server->WindowNewUpdate = freerds_client_inbound_window_new_update;
		connector->server->WindowDelete = freerds_client_inbound_window_delete;
		connector->server->LogonUser = freerds_client_inbound_logon_user;
		connector->server->LogoffUser = freerds_client_inbound_logoff_user;
	}

	freerds_message_server_connector_init(connector);

	return 0;
}
