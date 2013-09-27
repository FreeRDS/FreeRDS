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

int xrdp_server_message_enqueue(rdsModule* module, RDS_MSG_COMMON* msg)
{
	void* dup = NULL;
	rdsConnector* connector = (rdsConnector*) module;

	dup = xrdp_server_message_copy(msg);

	LinkedList_AddLast(connector->ServerList, (void*) dup);

	return 0;
}

/**
 * Server Callbacks
 */

int xrdp_message_server_is_terminated(rdsModule* module)
{
	int status;
	rdsConnector* connector = (rdsConnector*) module;
	status = connector->ServerProxy->IsTerminated(module);
	return status;
}

int xrdp_message_server_begin_update(rdsModule* module, RDS_MSG_BEGIN_UPDATE* msg)
{
	msg->type = RDS_SERVER_BEGIN_UPDATE;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_end_update(rdsModule* module, RDS_MSG_END_UPDATE* msg)
{
	msg->type = RDS_SERVER_END_UPDATE;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_beep(rdsModule* module, RDS_MSG_BEEP* msg)
{
	msg->type = RDS_SERVER_BEEP;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_opaque_rect(rdsModule* module, RDS_MSG_OPAQUE_RECT* msg)
{
	msg->type = RDS_SERVER_OPAQUE_RECT;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_screen_blt(rdsModule* module, RDS_MSG_SCREEN_BLT* msg)
{
	msg->type = RDS_SERVER_SCREEN_BLT;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_paint_rect(rdsModule* module, RDS_MSG_PAINT_RECT* msg)
{
	msg->type = RDS_SERVER_PAINT_RECT;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_patblt(rdsModule* module, RDS_MSG_PATBLT* msg)
{
	msg->type = RDS_SERVER_PATBLT;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_dstblt(rdsModule* module, RDS_MSG_DSTBLT* msg)
{
	msg->type = RDS_SERVER_DSTBLT;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_set_pointer(rdsModule* module, RDS_MSG_SET_POINTER* msg)
{
	msg->type = RDS_SERVER_SET_POINTER;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_set_palette(rdsModule* module, RDS_MSG_SET_PALETTE* msg)
{
	msg->type = RDS_SERVER_SET_PALETTE;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_set_clipping_region(rdsModule* module, RDS_MSG_SET_CLIPPING_REGION* msg)
{
	msg->type = RDS_SERVER_SET_CLIPPING_REGION;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_line_to(rdsModule* module, RDS_MSG_LINE_TO* msg)
{
	msg->type = RDS_SERVER_LINE_TO;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_cache_glyph(rdsModule* module, RDS_MSG_CACHE_GLYPH* msg)
{
	msg->type = RDS_SERVER_CACHE_GLYPH;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_glyph_index(rdsModule* module, RDS_MSG_GLYPH_INDEX* msg)
{
	msg->type = RDS_SERVER_GLYPH_INDEX;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_shared_framebuffer(rdsModule* module, RDS_MSG_SHARED_FRAMEBUFFER* msg)
{
	msg->type = RDS_SERVER_SHARED_FRAMEBUFFER;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_reset(rdsModule* module, RDS_MSG_RESET* msg)
{
	msg->type = RDS_SERVER_RESET;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_create_offscreen_surface(rdsModule* module, RDS_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_CREATE_OFFSCREEN_SURFACE;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_switch_offscreen_surface(rdsModule* module, RDS_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_SWITCH_OFFSCREEN_SURFACE;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_delete_offscreen_surface(rdsModule* module, RDS_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_DELETE_OFFSCREEN_SURFACE;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_paint_offscreen_surface(rdsModule* module, RDS_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_PAINT_OFFSCREEN_SURFACE;
	return xrdp_server_message_enqueue(module, (RDS_MSG_COMMON*) msg);
}

int xrdp_message_server_window_new_update(rdsModule* module, RDS_MSG_WINDOW_NEW_UPDATE* msg)
{
	//mod->ServerProxy->WindowNewUpdate(mod, msg);
	return 0;
}

int xrdp_message_server_window_delete(rdsModule* module, RDS_MSG_WINDOW_DELETE* msg)
{
	//mod->ServerProxy->WindowDelete(mod, msg);
	return 0;
}

int xrdp_message_server_queue_process_message(rdsModule* module, wMessage* message)
{
	int status;
	rdsConnector* connector;
	rdsServerInterface* ServerProxy;

	connector = (rdsConnector*) module;
	ServerProxy = connector->ServerProxy;

	if (message->id == WMQ_QUIT)
		return 0;

	switch (message->id)
	{
		case RDS_SERVER_BEGIN_UPDATE:
			status = ServerProxy->BeginUpdate(module, (RDS_MSG_BEGIN_UPDATE*) message->wParam);
			break;

		case RDS_SERVER_END_UPDATE:
			status = ServerProxy->EndUpdate(module, (RDS_MSG_END_UPDATE*) message->wParam);
			break;

		case RDS_SERVER_BEEP:
			status = ServerProxy->Beep(module, (RDS_MSG_BEEP*) message->wParam);
			break;

		case RDS_SERVER_OPAQUE_RECT:
			status = ServerProxy->OpaqueRect(module, (RDS_MSG_OPAQUE_RECT*) message->wParam);
			break;

		case RDS_SERVER_SCREEN_BLT:
			status = ServerProxy->ScreenBlt(module, (RDS_MSG_SCREEN_BLT*) message->wParam);
			break;

		case RDS_SERVER_PAINT_RECT:
			status = ServerProxy->PaintRect(module, (RDS_MSG_PAINT_RECT*) message->wParam);
			break;

		case RDS_SERVER_PATBLT:
			status = ServerProxy->PatBlt(module, (RDS_MSG_PATBLT*) message->wParam);
			break;

		case RDS_SERVER_DSTBLT:
			status = ServerProxy->DstBlt(module, (RDS_MSG_DSTBLT*) message->wParam);
			break;

		case RDS_SERVER_SET_POINTER:
			status = ServerProxy->SetPointer(module, (RDS_MSG_SET_POINTER*) message->wParam);
			break;

		case RDS_SERVER_SET_PALETTE:
			status = ServerProxy->SetPalette(module, (RDS_MSG_SET_PALETTE*) message->wParam);
			break;

		case RDS_SERVER_SET_CLIPPING_REGION:
			status = ServerProxy->SetClippingRegion(module, (RDS_MSG_SET_CLIPPING_REGION*) message->wParam);
			break;

		case RDS_SERVER_LINE_TO:
			status = ServerProxy->LineTo(module, (RDS_MSG_LINE_TO*) message->wParam);
			break;

		case RDS_SERVER_CACHE_GLYPH:
			status = ServerProxy->CacheGlyph(module, (RDS_MSG_CACHE_GLYPH*) message->wParam);
			break;

		case RDS_SERVER_GLYPH_INDEX:
			status = ServerProxy->GlyphIndex(module, (RDS_MSG_GLYPH_INDEX*) message->wParam);
			break;

		case RDS_SERVER_SHARED_FRAMEBUFFER:
			status = ServerProxy->SharedFramebuffer(module, (RDS_MSG_SHARED_FRAMEBUFFER*) message->wParam);
			break;

		case RDS_SERVER_RESET:
			status = ServerProxy->Reset(module, (RDS_MSG_RESET*) message->wParam);
			break;

		case RDS_SERVER_CREATE_OFFSCREEN_SURFACE:
			status = ServerProxy->CreateOffscreenSurface(module, (RDS_MSG_CREATE_OFFSCREEN_SURFACE*) message->wParam);
			break;

		case RDS_SERVER_SWITCH_OFFSCREEN_SURFACE:
			status = ServerProxy->SwitchOffscreenSurface(module, (RDS_MSG_SWITCH_OFFSCREEN_SURFACE*) message->wParam);
			break;

		case RDS_SERVER_DELETE_OFFSCREEN_SURFACE:
			status = ServerProxy->DeleteOffscreenSurface(module, (RDS_MSG_DELETE_OFFSCREEN_SURFACE*) message->wParam);
			break;

		case RDS_SERVER_PAINT_OFFSCREEN_SURFACE:
			status = ServerProxy->PaintOffscreenSurface(module, (RDS_MSG_PAINT_OFFSCREEN_SURFACE*) message->wParam);
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

int xrdp_message_server_align_rect(rdsModule* module, RDS_RECT* rect)
{
	rdpSettings* settings;
	rdsConnector* connector;

	connector = (rdsConnector*) module;
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

int xrdp_message_server_queue_pack(rdsModule* module)
{
	RDS_RECT rect;
	int ChainedMode;
	wLinkedList* list;
	rdsSession* session;
	RDS_MSG_COMMON* node;
	rdsConnector* connector;
	pixman_bool_t status;
	pixman_box32_t* extents;
	pixman_region32_t region;

	connector = (rdsConnector*) module;

	ChainedMode = 0;
	session = connector->session;

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
			MessageQueue_Post(connector->ServerQueue, (void*) module, node->type, (void*) node, NULL);
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

		xrdp_message_server_align_rect(module, &rect);

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

			MessageQueue_Post(connector->ServerQueue, (void*) module, msg->type, (void*) msg, NULL);
		}
	}

	pixman_region32_fini(&region);

	return 0;
}

int xrdp_message_server_queue_process_pending_messages(rdsModule* module)
{
	int count;
	int status;
	wMessage message;
	wMessageQueue* queue;
	rdsConnector* connector;

	connector = (rdsConnector*) module;

	count = 0;
	status = 0;
	queue = connector->ServerQueue;

	while (MessageQueue_Peek(queue, &message, TRUE))
	{
		status = xrdp_message_server_queue_process_message(module, &message);

		if (!status)
			break;

		count++;
	}

	return status;
}

int xrdp_message_server_module_init(rdsModule* module)
{
	rdsConnector* connector;

	connector = (rdsConnector*) module;

	connector->ServerProxy = (rdsServerInterface*) malloc(sizeof(rdsServerInterface));

	//mod->ServerProxy = NULL; /* disable */

	if (connector->ServerProxy)
	{
		CopyMemory(connector->ServerProxy, module->server, sizeof(rdsServerInterface));

		module->server->BeginUpdate = xrdp_message_server_begin_update;
		module->server->EndUpdate = xrdp_message_server_end_update;
		module->server->Beep = xrdp_message_server_beep;
		module->server->IsTerminated = xrdp_message_server_is_terminated;
		module->server->OpaqueRect = xrdp_message_server_opaque_rect;
		module->server->ScreenBlt = xrdp_message_server_screen_blt;
		module->server->PaintRect = xrdp_message_server_paint_rect;
		module->server->PatBlt = xrdp_message_server_patblt;
		module->server->DstBlt = xrdp_message_server_dstblt;
		module->server->SetPointer = xrdp_message_server_set_pointer;
		module->server->SetPalette = xrdp_message_server_set_palette;
		module->server->SetClippingRegion = xrdp_message_server_set_clipping_region;
		module->server->LineTo = xrdp_message_server_line_to;
		module->server->CacheGlyph = xrdp_message_server_cache_glyph;
		module->server->GlyphIndex = xrdp_message_glyph_index;
		module->server->SharedFramebuffer = xrdp_message_server_shared_framebuffer;
		module->server->Reset = xrdp_message_server_reset;
		module->server->CreateOffscreenSurface = xrdp_message_server_create_offscreen_surface;
		module->server->SwitchOffscreenSurface = xrdp_message_server_switch_offscreen_surface;
		module->server->DeleteOffscreenSurface = xrdp_message_server_delete_offscreen_surface;
		module->server->PaintOffscreenSurface = xrdp_message_server_paint_offscreen_surface;
		module->server->WindowNewUpdate = xrdp_message_server_window_new_update;
		module->server->WindowDelete = xrdp_message_server_window_delete;
	}

	connector->MaxFps = connector->fps = 60;
	connector->ServerList = LinkedList_New();
	connector->ServerQueue = MessageQueue_New();

	return 0;
}
