/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * Graphics Pipeline
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

#include "freerds.h"

int freerds_server_message_enqueue(rdsBackend* backend, RDS_MSG_COMMON* msg)
{
	void* dup = NULL;
	dup = freerds_server_message_copy(msg);

	LinkedList_AddLast(((rdsBackendConnector *)backend)->ServerList, (void*) dup);

	return 0;
}

/**
 * Server Callbacks
 */

int freerds_message_server_is_terminated(rdsBackend* backend)
{
	int status;
	status = ((rdsBackendConnector *)backend)->ServerProxy->IsTerminated(backend);
	return status;
}

int freerds_message_server_begin_update(rdsBackend* backend, RDS_MSG_BEGIN_UPDATE* msg)
{
	msg->type = RDS_SERVER_BEGIN_UPDATE;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_end_update(rdsBackend* backend, RDS_MSG_END_UPDATE* msg)
{
	msg->type = RDS_SERVER_END_UPDATE;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_beep(rdsBackend* backend, RDS_MSG_BEEP* msg)
{
	msg->type = RDS_SERVER_BEEP;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_opaque_rect(rdsBackend* backend, RDS_MSG_OPAQUE_RECT* msg)
{
	msg->type = RDS_SERVER_OPAQUE_RECT;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_screen_blt(rdsBackend* backend, RDS_MSG_SCREEN_BLT* msg)
{
	msg->type = RDS_SERVER_SCREEN_BLT;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_paint_rect(rdsBackend* backend, RDS_MSG_PAINT_RECT* msg)
{
	msg->type = RDS_SERVER_PAINT_RECT;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_patblt(rdsBackend* backend, RDS_MSG_PATBLT* msg)
{
	msg->type = RDS_SERVER_PATBLT;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_dstblt(rdsBackend* backend, RDS_MSG_DSTBLT* msg)
{
	msg->type = RDS_SERVER_DSTBLT;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_set_pointer(rdsBackend* backend, RDS_MSG_SET_POINTER* msg)
{
	msg->type = RDS_SERVER_SET_POINTER;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_set_system_pointer(rdsBackend* backend, RDS_MSG_SET_SYSTEM_POINTER* msg)
{
	msg->type = RDS_SERVER_SET_SYSTEM_POINTER;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_set_palette(rdsBackend* backend, RDS_MSG_SET_PALETTE* msg)
{
	msg->type = RDS_SERVER_SET_PALETTE;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_set_clipping_region(rdsBackend* backend, RDS_MSG_SET_CLIPPING_REGION* msg)
{
	msg->type = RDS_SERVER_SET_CLIPPING_REGION;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_line_to(rdsBackend* backend, RDS_MSG_LINE_TO* msg)
{
	msg->type = RDS_SERVER_LINE_TO;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_cache_glyph(rdsBackend* backend, RDS_MSG_CACHE_GLYPH* msg)
{
	msg->type = RDS_SERVER_CACHE_GLYPH;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_glyph_index(rdsBackend* backend, RDS_MSG_GLYPH_INDEX* msg)
{
	msg->type = RDS_SERVER_GLYPH_INDEX;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_shared_framebuffer(rdsBackend* backend, RDS_MSG_SHARED_FRAMEBUFFER* msg)
{
	msg->type = RDS_SERVER_SHARED_FRAMEBUFFER;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_reset(rdsBackend* backend, RDS_MSG_RESET* msg)
{
	msg->type = RDS_SERVER_RESET;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_create_offscreen_surface(rdsBackend* backend, RDS_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_CREATE_OFFSCREEN_SURFACE;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_switch_offscreen_surface(rdsBackend* backend, RDS_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_SWITCH_OFFSCREEN_SURFACE;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_delete_offscreen_surface(rdsBackend* backend, RDS_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_DELETE_OFFSCREEN_SURFACE;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_paint_offscreen_surface(rdsBackend* backend, RDS_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_PAINT_OFFSCREEN_SURFACE;
	return freerds_server_message_enqueue(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_message_server_window_new_update(rdsBackend* backend, RDS_MSG_WINDOW_NEW_UPDATE* msg)
{
	//mod->ServerProxy->WindowNewUpdate(mod, msg);
	return 0;
}

int freerds_message_server_window_delete(rdsBackend* backend, RDS_MSG_WINDOW_DELETE* msg)
{
	//mod->ServerProxy->WindowDelete(mod, msg);
	return 0;
}

int freerds_message_server_queue_process_message(rdsBackendConnector* connector, wMessage* message)
{
	int status;
	rdsServerInterface* ServerProxy;
	rdsBackend *backend = (rdsBackend *)connector;

	ServerProxy = connector->ServerProxy;

	if (message->id == WMQ_QUIT)
		return 0;

	switch (message->id)
	{
		case RDS_SERVER_BEGIN_UPDATE:
			status = ServerProxy->BeginUpdate(backend, (RDS_MSG_BEGIN_UPDATE*) message->wParam);
			break;

		case RDS_SERVER_END_UPDATE:
			status = ServerProxy->EndUpdate(backend, (RDS_MSG_END_UPDATE*) message->wParam);
			break;

		case RDS_SERVER_BEEP:
			status = ServerProxy->Beep(backend, (RDS_MSG_BEEP*) message->wParam);
			break;

		case RDS_SERVER_OPAQUE_RECT:
			status = ServerProxy->OpaqueRect(backend, (RDS_MSG_OPAQUE_RECT*) message->wParam);
			break;

		case RDS_SERVER_SCREEN_BLT:
			status = ServerProxy->ScreenBlt(backend, (RDS_MSG_SCREEN_BLT*) message->wParam);
			break;

		case RDS_SERVER_PAINT_RECT:
			status = ServerProxy->PaintRect(backend, (RDS_MSG_PAINT_RECT*) message->wParam);
			break;

		case RDS_SERVER_PATBLT:
			status = ServerProxy->PatBlt(backend, (RDS_MSG_PATBLT*) message->wParam);
			break;

		case RDS_SERVER_DSTBLT:
			status = ServerProxy->DstBlt(backend, (RDS_MSG_DSTBLT*) message->wParam);
			break;

		case RDS_SERVER_SET_POINTER:
			status = ServerProxy->SetPointer(backend, (RDS_MSG_SET_POINTER*) message->wParam);
			break;

		case RDS_SERVER_SET_SYSTEM_POINTER:
			status = ServerProxy->SetSystemPointer(backend, (RDS_MSG_SET_SYSTEM_POINTER*) message->wParam);
			break;

		case RDS_SERVER_SET_PALETTE:
			status = ServerProxy->SetPalette(backend, (RDS_MSG_SET_PALETTE*) message->wParam);
			break;

		case RDS_SERVER_SET_CLIPPING_REGION:
			status = ServerProxy->SetClippingRegion(backend, (RDS_MSG_SET_CLIPPING_REGION*) message->wParam);
			break;

		case RDS_SERVER_LINE_TO:
			status = ServerProxy->LineTo(backend, (RDS_MSG_LINE_TO*) message->wParam);
			break;

		case RDS_SERVER_CACHE_GLYPH:
			status = ServerProxy->CacheGlyph(backend, (RDS_MSG_CACHE_GLYPH*) message->wParam);
			break;

		case RDS_SERVER_GLYPH_INDEX:
			status = ServerProxy->GlyphIndex(backend, (RDS_MSG_GLYPH_INDEX*) message->wParam);
			break;

		case RDS_SERVER_SHARED_FRAMEBUFFER:
			status = ServerProxy->SharedFramebuffer(backend, (RDS_MSG_SHARED_FRAMEBUFFER*) message->wParam);
			break;

		case RDS_SERVER_RESET:
			status = ServerProxy->Reset(backend, (RDS_MSG_RESET*) message->wParam);
			break;

		case RDS_SERVER_CREATE_OFFSCREEN_SURFACE:
			status = ServerProxy->CreateOffscreenSurface(backend, (RDS_MSG_CREATE_OFFSCREEN_SURFACE*) message->wParam);
			break;

		case RDS_SERVER_SWITCH_OFFSCREEN_SURFACE:
			status = ServerProxy->SwitchOffscreenSurface(backend, (RDS_MSG_SWITCH_OFFSCREEN_SURFACE*) message->wParam);
			break;

		case RDS_SERVER_DELETE_OFFSCREEN_SURFACE:
			status = ServerProxy->DeleteOffscreenSurface(backend, (RDS_MSG_DELETE_OFFSCREEN_SURFACE*) message->wParam);
			break;

		case RDS_SERVER_PAINT_OFFSCREEN_SURFACE:
			status = ServerProxy->PaintOffscreenSurface(backend, (RDS_MSG_PAINT_OFFSCREEN_SURFACE*) message->wParam);
			break;

		default:
			status = -1;
			break;
	}

	freerds_server_message_free((RDS_MSG_COMMON*) message->wParam);

	if (status < 0)
	{
		printf("freerds_message_server_queue_process_message (%d) status: %d\n", message->id, status);
		return -1;
	}

	return 0;
}

int freerds_message_server_align_rect(rdsBackendConnector* connector, RDS_RECT* rect)
{
	rdpSettings* settings;
	settings = connector->settings;

	if (rect->x < 0)
		rect->x = 0;

	if (rect->x > settings->DesktopWidth - 1)
		rect->x = settings->DesktopWidth - 1;

	rect->width += rect->x % 16;
	rect->x -= rect->x % 16;

	rect->width += rect->width % 16;

	if (rect->x + rect->width > settings->DesktopWidth)
		rect->width = settings->DesktopWidth - rect->x;

	if (rect->y < 0)
		rect->y = 0;

	if (rect->y > settings->DesktopHeight - 1)
		rect->y = settings->DesktopHeight - 1;

	rect->height += rect->y % 16;
	rect->y -= rect->y % 16;

	rect->height += rect->height % 16;

	if (rect->height > settings->DesktopHeight)
		rect->height = settings->DesktopHeight;

	if (rect->y + rect->height > settings->DesktopHeight)
		rect->height = settings->DesktopHeight - rect->y;

	return 0;
}

int freerds_message_server_queue_pack(rdsBackendConnector* connector)
{
	RDS_RECT rect;
	int ChainedMode;
	wLinkedList* list;
	rdsConnection* connection;
	RDS_MSG_COMMON* node;
	pixman_bool_t status;
	pixman_box32_t* extents;
	pixman_region32_t region;

	ChainedMode = 0;
	connection = connector->connection;

	list = connector->ServerList;

	pixman_region32_init(&region);

	LinkedList_Enumerator_Reset(list);

	while (LinkedList_Enumerator_MoveNext(list))
	{
		node = (RDS_MSG_COMMON*) LinkedList_Enumerator_Current(list);

		if ((!ChainedMode) && (node->msgFlags & RDS_MSG_FLAG_RECT))
		{
			status = pixman_region32_union_rect(&region, &region,
					node->rect.x, node->rect.y, node->rect.width, node->rect.height);
		}
		else
		{
			MessageQueue_Post(connector->ServerQueue, (void*) connector, node->type, (void*) node, NULL);
		}
	}

	LinkedList_Clear(list);

	if (!ChainedMode)
	{
		extents = pixman_region32_extents(&region);

		rect.x = extents->x1;
		rect.y = extents->y1;
		rect.width = extents->x2 - extents->x1;
		rect.height = extents->y2 - extents->y1;

		freerds_message_server_align_rect(connector, &rect);

		if (connector->framebuffer.fbAttached && (rect.width * rect.height))
		{
			RDS_MSG_COMMON* msg;
			RDS_MSG_PAINT_RECT paintRect;

			paintRect.type = RDS_SERVER_PAINT_RECT;

			paintRect.nXSrc = 0;
			paintRect.nYSrc = 0;
			paintRect.bitmapData = NULL;
			paintRect.bitmapDataLength = 0;
			paintRect.framebuffer = &(connector->framebuffer);
			paintRect.fbSegmentId = connector->framebuffer.fbSegmentId;

			paintRect.nLeftRect = rect.x;
			paintRect.nTopRect = rect.y;
			paintRect.nWidth = rect.width;
			paintRect.nHeight = rect.height;

			msg = freerds_server_message_copy((RDS_MSG_COMMON*) &paintRect);

			MessageQueue_Post(connector->ServerQueue, (void*) connector, msg->type, (void*) msg, NULL);
		}
	}

	pixman_region32_fini(&region);

	return 0;
}

int freerds_message_server_queue_process_pending_messages(rdsBackendConnector* connector)
{
	int count;
	int status;
	wMessage message;
	wMessageQueue* queue;

	count = 0;
	status = 0;
	queue = connector->ServerQueue;

	while (MessageQueue_Peek(queue, &message, TRUE))
	{
		status = freerds_message_server_queue_process_message(connector, &message);

		if (!status)
			break;

		count++;
	}

	return status;
}

int freerds_message_server_connector_init(rdsBackendConnector* backend)
{
	rdsBackendConnector* connector = (rdsBackendConnector *)backend;
	connector->ServerProxy = (rdsServerInterface*) malloc(sizeof(rdsServerInterface));

	//mod->ServerProxy = NULL; /* disable */

	if (connector->ServerProxy)
	{
		CopyMemory(connector->ServerProxy, connector->server, sizeof(rdsServerInterface));

		connector->server->BeginUpdate = freerds_message_server_begin_update;
		connector->server->EndUpdate = freerds_message_server_end_update;
		connector->server->Beep = freerds_message_server_beep;
		connector->server->IsTerminated = freerds_message_server_is_terminated;
		connector->server->OpaqueRect = freerds_message_server_opaque_rect;
		connector->server->ScreenBlt = freerds_message_server_screen_blt;
		connector->server->PaintRect = freerds_message_server_paint_rect;
		connector->server->PatBlt = freerds_message_server_patblt;
		connector->server->DstBlt = freerds_message_server_dstblt;
		connector->server->SetPointer = freerds_message_server_set_pointer;
		connector->server->SetSystemPointer = freerds_message_server_set_system_pointer;
		connector->server->SetPalette = freerds_message_server_set_palette;
		connector->server->SetClippingRegion = freerds_message_server_set_clipping_region;
		connector->server->LineTo = freerds_message_server_line_to;
		connector->server->CacheGlyph = freerds_message_server_cache_glyph;
		connector->server->GlyphIndex = freerds_message_glyph_index;
		connector->server->SharedFramebuffer = freerds_message_server_shared_framebuffer;
		connector->server->Reset = freerds_message_server_reset;
		connector->server->CreateOffscreenSurface = freerds_message_server_create_offscreen_surface;
		connector->server->SwitchOffscreenSurface = freerds_message_server_switch_offscreen_surface;
		connector->server->DeleteOffscreenSurface = freerds_message_server_delete_offscreen_surface;
		connector->server->PaintOffscreenSurface = freerds_message_server_paint_offscreen_surface;
		connector->server->WindowNewUpdate = freerds_message_server_window_new_update;
		connector->server->WindowDelete = freerds_message_server_window_delete;
	}

	connector->fps = 10;
	connector->MaxFps = 30;

	connector->ServerList = LinkedList_New();
	connector->ServerQueue = MessageQueue_New(NULL);

	return 0;
}
