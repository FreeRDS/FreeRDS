/**
 * xrdp: A Remote Desktop Protocol server.
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

#include "xrdp.h"

int xrdp_server_message_enqueue(rdsModuleConnector* connector, RDS_MSG_COMMON* msg)
{
	void* dup = NULL;
	dup = xrdp_server_message_copy(msg);

	LinkedList_AddLast(connector->ServerList, (void*) dup);

	return 0;
}

/**
 * Server Callbacks
 */

int xrdp_message_server_is_terminated(rdsModuleConnector* connector)
{
	int status;
	status = connector->ServerProxy->IsTerminated(connector);
	return status;
}

int xrdp_message_server_begin_update(rdsModuleConnector* connector, RDS_MSG_BEGIN_UPDATE* msg)
{
	msg->type = RDS_SERVER_BEGIN_UPDATE;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_end_update(rdsModuleConnector* connector, RDS_MSG_END_UPDATE* msg)
{
	msg->type = RDS_SERVER_END_UPDATE;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_beep(rdsModuleConnector* connector, RDS_MSG_BEEP* msg)
{
	msg->type = RDS_SERVER_BEEP;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_opaque_rect(rdsModuleConnector* connector, RDS_MSG_OPAQUE_RECT* msg)
{
	msg->type = RDS_SERVER_OPAQUE_RECT;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_screen_blt(rdsModuleConnector* connector, RDS_MSG_SCREEN_BLT* msg)
{
	msg->type = RDS_SERVER_SCREEN_BLT;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_paint_rect(rdsModuleConnector* connector, RDS_MSG_PAINT_RECT* msg)
{
	msg->type = RDS_SERVER_PAINT_RECT;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_patblt(rdsModuleConnector* connector, RDS_MSG_PATBLT* msg)
{
	msg->type = RDS_SERVER_PATBLT;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_dstblt(rdsModuleConnector* connector, RDS_MSG_DSTBLT* msg)
{
	msg->type = RDS_SERVER_DSTBLT;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_set_pointer(rdsModuleConnector* connector, RDS_MSG_SET_POINTER* msg)
{
	msg->type = RDS_SERVER_SET_POINTER;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_set_system_pointer(rdsModuleConnector* connector, RDS_MSG_SET_SYSTEM_POINTER* msg)
{
	msg->type = RDS_SERVER_SET_SYSTEM_POINTER;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_set_palette(rdsModuleConnector* connector, RDS_MSG_SET_PALETTE* msg)
{
	msg->type = RDS_SERVER_SET_PALETTE;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_set_clipping_region(rdsModuleConnector* connector, RDS_MSG_SET_CLIPPING_REGION* msg)
{
	msg->type = RDS_SERVER_SET_CLIPPING_REGION;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_line_to(rdsModuleConnector* connector, RDS_MSG_LINE_TO* msg)
{
	msg->type = RDS_SERVER_LINE_TO;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_cache_glyph(rdsModuleConnector* connector, RDS_MSG_CACHE_GLYPH* msg)
{
	msg->type = RDS_SERVER_CACHE_GLYPH;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_glyph_index(rdsModuleConnector* connector, RDS_MSG_GLYPH_INDEX* msg)
{
	msg->type = RDS_SERVER_GLYPH_INDEX;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_shared_framebuffer(rdsModuleConnector* connector, RDS_MSG_SHARED_FRAMEBUFFER* msg)
{
	msg->type = RDS_SERVER_SHARED_FRAMEBUFFER;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_reset(rdsModuleConnector* connector, RDS_MSG_RESET* msg)
{
	msg->type = RDS_SERVER_RESET;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_create_offscreen_surface(rdsModuleConnector* connector, RDS_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_CREATE_OFFSCREEN_SURFACE;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_switch_offscreen_surface(rdsModuleConnector* connector, RDS_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_SWITCH_OFFSCREEN_SURFACE;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_delete_offscreen_surface(rdsModuleConnector* connector, RDS_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_DELETE_OFFSCREEN_SURFACE;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_paint_offscreen_surface(rdsModuleConnector* connector, RDS_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_PAINT_OFFSCREEN_SURFACE;
	return xrdp_server_message_enqueue(connector, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_window_new_update(rdsModuleConnector* connector, RDS_MSG_WINDOW_NEW_UPDATE* msg)
{
	//mod->ServerProxy->WindowNewUpdate(mod, msg);
	return 0;
}

int xrdp_message_server_window_delete(rdsModuleConnector* connector, RDS_MSG_WINDOW_DELETE* msg)
{
	//mod->ServerProxy->WindowDelete(mod, msg);
	return 0;
}

int xrdp_message_server_queue_process_message(rdsModuleConnector* connector, wMessage* message)
{
	int status;
	rdsServerInterface* ServerProxy;

	ServerProxy = connector->ServerProxy;

	if (message->id == WMQ_QUIT)
		return 0;

	switch (message->id)
	{
		case RDS_SERVER_BEGIN_UPDATE:
			status = ServerProxy->BeginUpdate(connector, (RDS_MSG_BEGIN_UPDATE*) message->wParam);
			break;

		case RDS_SERVER_END_UPDATE:
			status = ServerProxy->EndUpdate(connector, (RDS_MSG_END_UPDATE*) message->wParam);
			break;

		case RDS_SERVER_BEEP:
			status = ServerProxy->Beep(connector, (RDS_MSG_BEEP*) message->wParam);
			break;

		case RDS_SERVER_OPAQUE_RECT:
			status = ServerProxy->OpaqueRect(connector, (RDS_MSG_OPAQUE_RECT*) message->wParam);
			break;

		case RDS_SERVER_SCREEN_BLT:
			status = ServerProxy->ScreenBlt(connector, (RDS_MSG_SCREEN_BLT*) message->wParam);
			break;

		case RDS_SERVER_PAINT_RECT:
			status = ServerProxy->PaintRect(connector, (RDS_MSG_PAINT_RECT*) message->wParam);
			break;

		case RDS_SERVER_PATBLT:
			status = ServerProxy->PatBlt(connector, (RDS_MSG_PATBLT*) message->wParam);
			break;

		case RDS_SERVER_DSTBLT:
			status = ServerProxy->DstBlt(connector, (RDS_MSG_DSTBLT*) message->wParam);
			break;

		case RDS_SERVER_SET_POINTER:
			status = ServerProxy->SetPointer(connector, (RDS_MSG_SET_POINTER*) message->wParam);
			break;

		case RDS_SERVER_SET_SYSTEM_POINTER:
			status = ServerProxy->SetSystemPointer(connector, (RDS_MSG_SET_SYSTEM_POINTER*) message->wParam);
			break;

		case RDS_SERVER_SET_PALETTE:
			status = ServerProxy->SetPalette(connector, (RDS_MSG_SET_PALETTE*) message->wParam);
			break;

		case RDS_SERVER_SET_CLIPPING_REGION:
			status = ServerProxy->SetClippingRegion(connector, (RDS_MSG_SET_CLIPPING_REGION*) message->wParam);
			break;

		case RDS_SERVER_LINE_TO:
			status = ServerProxy->LineTo(connector, (RDS_MSG_LINE_TO*) message->wParam);
			break;

		case RDS_SERVER_CACHE_GLYPH:
			status = ServerProxy->CacheGlyph(connector, (RDS_MSG_CACHE_GLYPH*) message->wParam);
			break;

		case RDS_SERVER_GLYPH_INDEX:
			status = ServerProxy->GlyphIndex(connector, (RDS_MSG_GLYPH_INDEX*) message->wParam);
			break;

		case RDS_SERVER_SHARED_FRAMEBUFFER:
			status = ServerProxy->SharedFramebuffer(connector, (RDS_MSG_SHARED_FRAMEBUFFER*) message->wParam);
			break;

		case RDS_SERVER_RESET:
			status = ServerProxy->Reset(connector, (RDS_MSG_RESET*) message->wParam);
			break;

		case RDS_SERVER_CREATE_OFFSCREEN_SURFACE:
			status = ServerProxy->CreateOffscreenSurface(connector, (RDS_MSG_CREATE_OFFSCREEN_SURFACE*) message->wParam);
			break;

		case RDS_SERVER_SWITCH_OFFSCREEN_SURFACE:
			status = ServerProxy->SwitchOffscreenSurface(connector, (RDS_MSG_SWITCH_OFFSCREEN_SURFACE*) message->wParam);
			break;

		case RDS_SERVER_DELETE_OFFSCREEN_SURFACE:
			status = ServerProxy->DeleteOffscreenSurface(connector, (RDS_MSG_DELETE_OFFSCREEN_SURFACE*) message->wParam);
			break;

		case RDS_SERVER_PAINT_OFFSCREEN_SURFACE:
			status = ServerProxy->PaintOffscreenSurface(connector, (RDS_MSG_PAINT_OFFSCREEN_SURFACE*) message->wParam);
			break;

		default:
			status = -1;
			break;
	}

	xrdp_server_message_free((RDS_MSG_COMMON*) message->wParam);

	if (status < 0)
	{
		printf("xrdp_message_server_queue_process_message (%d) status: %d\n", message->id, status);
		return -1;
	}

	return 0;
}

int xrdp_message_server_align_rect(rdsModuleConnector* connector, RDS_RECT* rect)
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

int xrdp_message_server_queue_pack(rdsModuleConnector* connector)
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

		xrdp_message_server_align_rect(connector, &rect);

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

			msg = xrdp_server_message_copy((RDS_MSG_COMMON*) &paintRect);

			MessageQueue_Post(connector->ServerQueue, (void*) connector, msg->type, (void*) msg, NULL);
		}
	}

	pixman_region32_fini(&region);

	return 0;
}

int xrdp_message_server_queue_process_pending_messages(rdsModuleConnector* connector)
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
		status = xrdp_message_server_queue_process_message(connector, &message);

		if (!status)
			break;

		count++;
	}

	return status;
}

int xrdp_message_server_connector_init(rdsModuleConnector* connector)
{
	connector->ServerProxy = (rdsServerInterface*) malloc(sizeof(rdsServerInterface));

	//mod->ServerProxy = NULL; /* disable */

	if (connector->ServerProxy)
	{
		CopyMemory(connector->ServerProxy, connector->server, sizeof(rdsServerInterface));

		connector->server->BeginUpdate = xrdp_message_server_begin_update;
		connector->server->EndUpdate = xrdp_message_server_end_update;
		connector->server->Beep = xrdp_message_server_beep;
		connector->server->IsTerminated = xrdp_message_server_is_terminated;
		connector->server->OpaqueRect = xrdp_message_server_opaque_rect;
		connector->server->ScreenBlt = xrdp_message_server_screen_blt;
		connector->server->PaintRect = xrdp_message_server_paint_rect;
		connector->server->PatBlt = xrdp_message_server_patblt;
		connector->server->DstBlt = xrdp_message_server_dstblt;
		connector->server->SetPointer = xrdp_message_server_set_pointer;
		connector->server->SetSystemPointer = xrdp_message_server_set_system_pointer;
		connector->server->SetPalette = xrdp_message_server_set_palette;
		connector->server->SetClippingRegion = xrdp_message_server_set_clipping_region;
		connector->server->LineTo = xrdp_message_server_line_to;
		connector->server->CacheGlyph = xrdp_message_server_cache_glyph;
		connector->server->GlyphIndex = xrdp_message_glyph_index;
		connector->server->SharedFramebuffer = xrdp_message_server_shared_framebuffer;
		connector->server->Reset = xrdp_message_server_reset;
		connector->server->CreateOffscreenSurface = xrdp_message_server_create_offscreen_surface;
		connector->server->SwitchOffscreenSurface = xrdp_message_server_switch_offscreen_surface;
		connector->server->DeleteOffscreenSurface = xrdp_message_server_delete_offscreen_surface;
		connector->server->PaintOffscreenSurface = xrdp_message_server_paint_offscreen_surface;
		connector->server->WindowNewUpdate = xrdp_message_server_window_new_update;
		connector->server->WindowDelete = xrdp_message_server_window_delete;
	}

	connector->MaxFps = connector->fps = 60;
	connector->ServerList = LinkedList_New();
	connector->ServerQueue = MessageQueue_New();

	return 0;
}
