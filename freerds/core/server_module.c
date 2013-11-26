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

#include <freerds/backend.h>
#include <freerds/icp_client_stubs.h>
#include <freerds/icp.h>
#include "../icp/pbrpc/pbrpc.h"
#include "pbRPC.pb-c.h"

int freerds_client_inbound_begin_update(rdsBackend* backend, RDS_MSG_BEGIN_UPDATE* msg)
{
	freerds_orders_begin_paint(((rdsBackendConnector *)backend)->connection);
	return 0;
}

int freerds_client_inbound_end_update(rdsBackend* backend, RDS_MSG_END_UPDATE* msg)
{
	freerds_orders_end_paint(((rdsBackendConnector *)backend)->connection);
	backend->client->VBlankEvent(backend);
	return 0;
}

int freerds_client_inbound_beep(rdsBackend* backend, RDS_MSG_BEEP* msg)
{
	freerds_send_bell(((rdsBackendConnector *)backend)->connection);
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
	rdsBackendConnector *connector = (rdsBackendConnector *)backend;

	connection = connector->connection;
	settings = connection->settings;

	if (connection->codecMode)
	{
		bpp = msg->framebuffer->fbBitsPerPixel;

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
	freerds_set_pointer(((rdsBackendConnector *)backend)->connection, msg);
	return 0;
}

int freerds_client_inbound_set_system_pointer(rdsBackend* backend, RDS_MSG_SET_SYSTEM_POINTER* msg)
{
	freerds_set_system_pointer(((rdsBackendConnector *)backend)->connection, msg);
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
	return freerds_orders_send_font(((rdsBackendConnector *)backend)->connection, msg);
}

int freerds_client_inbound_glyph_index(rdsBackend* backend, RDS_MSG_GLYPH_INDEX* msg)
{
	/* TODO */

	return 0;
}

int freerds_client_inbound_shared_framebuffer(rdsBackend* backend, RDS_MSG_SHARED_FRAMEBUFFER* msg)
{
	backend->framebuffer.fbWidth = msg->width;
	backend->framebuffer.fbHeight = msg->height;
	backend->framebuffer.fbScanline = msg->scanline;
	backend->framebuffer.fbSegmentId = msg->segmentId;
	backend->framebuffer.fbBitsPerPixel = msg->bitsPerPixel;
	backend->framebuffer.fbBytesPerPixel = msg->bytesPerPixel;

	printf("received shared framebuffer message: mod->framebuffer.fbAttached: %d msg->attach: %d\n",
			backend->framebuffer.fbAttached, msg->attach);

	if (!backend->framebuffer.fbAttached && msg->attach)
	{
		RDS_MSG_PAINT_RECT fm;

		backend->framebuffer.fbSharedMemory = (BYTE*) shmat(backend->framebuffer.fbSegmentId, 0, 0);
		backend->framebuffer.fbAttached = TRUE;

		printf("attached segment %d to %p\n",
				backend->framebuffer.fbSegmentId, backend->framebuffer.fbSharedMemory);

		backend->framebuffer.image = (void*) pixman_image_create_bits(PIXMAN_x8r8g8b8,
				backend->framebuffer.fbWidth, backend->framebuffer.fbHeight,
				(uint32_t*) backend->framebuffer.fbSharedMemory, backend->framebuffer.fbScanline);

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

	if (backend->framebuffer.fbAttached && !msg->attach)
	{
		printf("detaching segment %d to %p\n",
				backend->framebuffer.fbSegmentId, backend->framebuffer.fbSharedMemory);
		shmdt(backend->framebuffer.fbSharedMemory);
		backend->framebuffer.fbAttached = FALSE;
		backend->framebuffer.fbSharedMemory = 0;
	}

	backend->client->VBlankEvent(backend);

	return 0;
}

int freerds_client_inbound_reset(rdsBackend* backend, RDS_MSG_RESET* msg)
{
	if (freerds_reset(((rdsBackendConnector *)backend)->connection, msg) != 0)
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
	return freerds_window_new_update(((rdsBackendConnector *)backend)->connection, msg);
}

int freerds_client_inbound_window_delete(rdsBackend* backend, RDS_MSG_WINDOW_DELETE* msg)
{
	return freerds_window_delete(((rdsBackendConnector *)backend)->connection, msg);
}

int freerds_client_inbound_logon_user(rdsBackend* backend, RDS_MSG_LOGON_USER* msg)
{
	int icpStatus;
	int authStatus;
	char* endPoint;
	HANDLE hClientPipe;
	rdsBackendConnector* connector = (rdsBackendConnector*)backend;
	rdsConnection* connection = connector->connection;

	if (msg->Domain)
		fprintf(stderr, "LogonUser: %s\\%s | %s", msg->Domain, msg->User, msg->Password);
	else
		fprintf(stderr, "LogonUser: %s | %s", msg->User, msg->Password);

	authStatus = 0;
	endPoint = NULL;

	icpStatus = freerds_icp_LogonUser(connection->id,
			msg->User, msg->Domain, msg->Password, &endPoint);

	if (icpStatus != 0)
	{
		printf("freerds_icp_LogonUser failed %d\n", icpStatus);
		return -1;
	}

	fprintf(stderr, "Logon Status: %d\n", authStatus);

	freerds_connector_free(connector);
	connection->connector = freerds_connector_new(connection);
	connector->Endpoint = endPoint;

	hClientPipe = freerds_named_pipe_connect(connector->Endpoint, 20);

	if (hClientPipe == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Failed to create named pipe %s\n", connector->Endpoint);
		return FALSE;
	}

	printf("Connected to session %s\n",  connector->Endpoint);

	if (freerds_init_client(hClientPipe, connection->settings, connector->OutboundStream) < 0)
	{
		fprintf(stderr, "Error sending initial packet with %s\n", connector->Endpoint);
		return FALSE;
	}

	connector->hClientPipe = hClientPipe;
	connector->GetEventHandles = freerds_client_get_event_handles;
	connector->CheckEventHandles = freerds_client_check_event_handles;

	connector->ServerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) freerds_client_thread,
			(void*) connector, CREATE_SUSPENDED, NULL);

	freerds_client_inbound_connector_init(connector);

	ResumeThread(connector->ServerThread);

	if (authStatus != 0)
	{
		RDS_MSG_LOGON_USER logonUser;

		ZeroMemory(&logonUser, sizeof(RDS_MSG_LOGON_USER));
		logonUser.type = RDS_CLIENT_LOGON_USER;

		logonUser.Flags = 0;
		logonUser.User = msg->User;
		logonUser.Domain = msg->Domain;
		logonUser.Password = msg->Password;

		connector->client->LogonUser((rdsBackend *)connector, &logonUser);
	}

	return 0;
}

int freerds_client_inbound_logoff_user(rdsBackend* backend, RDS_MSG_LOGOFF_USER* msg)
{
	return 0;
}


struct _icps_context {
	rdsBackend* backend;
};
typedef struct _icps_context IcpsContext;

static IcpsContext *IcpsContext_new(rdsBackend* backend)
{
	IcpsContext *ret = (IcpsContext *)malloc(sizeof(IcpsContext));
	ret->backend = backend;
	return ret;
}

void icpsCallback(UINT32 reason, Freerds__Pbrpc__RPCBase* response, void *args) {
	IcpsContext *context = (IcpsContext *)args;
	RDS_MSG_ICPS_REPLY icps;
	rdsBackend *backend = context->backend;

	ZeroMemory(&icps, sizeof(icps));

	switch(reason) {
	case PBRPC_SUCCESS:
		if (!response)
		{
			fprintf(stderr, "%s: expecting a response and don't have one\n", __FUNCTION__);
			goto cleanup_exit;
		}

		/*
		if (response->msgtype < FREERDS__ICPS__MSGTYPE__FirstMessage ||
				response->msgtype > FREERDS__ICPS__MSGTYPE__LastMessage)
		{
			fprintf(stderr, "received message is not an ICPS message, not forwarding\n");
			goto cleanup_exit;
		}*/

		icps.status = ICPS_REPLY_SUCCESS;
		icps.icpsType = response->msgtype;
		icps.dataLen = response->payload.len;
		icps.data = (char *)response->payload.data;
		break;

	case PBRCP_TRANSPORT_ERROR:
	default:
		icps.status = ICPS_REPLY_TRANSPORT_ERROR;
		break;
	}

	backend->client->Icps(backend, &icps);

cleanup_exit:
	free(context);
}

int freerds_client_inbound_icps(rdsBackend* backend, RDS_MSG_ICPS_REQUEST* msg)
{
	IcpsContext *icpsContext = IcpsContext_new(backend);

	pbRPCPayload payload;
	payload.data = msg->data;
	payload.dataLen = msg->dataLen;
	payload.errorDescription = 0;

	pbRPCContext *pbContext = (pbRPCContext *)freerds_icp_get_context();

	pbrcp_call_method_async(pbContext, msg->icpsType, &payload, icpsCallback, (void *)icpsContext);
	return 0;
}


int freerds_client_inbound_connector_init(rdsBackendConnector* connector)
{
	rdsServerInterface *serverInter;

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
		serverInter->Icps = freerds_client_inbound_icps;
	}

	freerds_message_server_connector_init(connector);

	return 0;
}
