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

int xrdp_server_message_enqueue(xrdpModule* mod, XRDP_MSG_COMMON* msg)
{
	void* dup = NULL;

	dup = xrdp_server_message_copy(msg);

	LinkedList_AddLast(mod->ServerList, (void*) dup);

	return 0;
}

/**
 * Server Callbacks
 */

int xrdp_message_server_is_terminated(xrdpModule* mod)
{
	int status;
	status = mod->ServerProxy->IsTerminated(mod);
	return status;
}

int xrdp_message_server_begin_update(xrdpModule* mod, XRDP_MSG_BEGIN_UPDATE* msg)
{
	msg->type = XRDP_SERVER_BEGIN_UPDATE;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_end_update(xrdpModule* mod, XRDP_MSG_END_UPDATE* msg)
{
	msg->type = XRDP_SERVER_END_UPDATE;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_beep(xrdpModule* mod, XRDP_MSG_BEEP* msg)
{
	msg->type = XRDP_SERVER_BEEP;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_opaque_rect(xrdpModule* mod, XRDP_MSG_OPAQUE_RECT* msg)
{
	msg->type = XRDP_SERVER_OPAQUE_RECT;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_screen_blt(xrdpModule* mod, XRDP_MSG_SCREEN_BLT* msg)
{
	msg->type = XRDP_SERVER_SCREEN_BLT;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_paint_rect(xrdpModule* mod, XRDP_MSG_PAINT_RECT* msg)
{
	msg->type = XRDP_SERVER_PAINT_RECT;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_patblt(xrdpModule* mod, XRDP_MSG_PATBLT* msg)
{
	msg->type = XRDP_SERVER_PATBLT;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_dstblt(xrdpModule* mod, XRDP_MSG_DSTBLT* msg)
{
	msg->type = XRDP_SERVER_DSTBLT;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_set_pointer(xrdpModule* mod, XRDP_MSG_SET_POINTER* msg)
{
	msg->type = XRDP_SERVER_SET_POINTER;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_set_palette(xrdpModule* mod, XRDP_MSG_SET_PALETTE* msg)
{
	msg->type = XRDP_SERVER_SET_PALETTE;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_set_clipping_region(xrdpModule* mod, XRDP_MSG_SET_CLIPPING_REGION* msg)
{
	msg->type = XRDP_SERVER_SET_CLIPPING_REGION;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_line_to(xrdpModule* mod, XRDP_MSG_LINE_TO* msg)
{
	msg->type = XRDP_SERVER_LINE_TO;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_cache_glyph(xrdpModule* mod, XRDP_MSG_CACHE_GLYPH* msg)
{
	msg->type = XRDP_SERVER_CACHE_GLYPH;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_glyph_index(xrdpModule* mod, XRDP_MSG_GLYPH_INDEX* msg)
{
	msg->type = XRDP_SERVER_GLYPH_INDEX;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_shared_framebuffer(xrdpModule* mod, XRDP_MSG_SHARED_FRAMEBUFFER* msg)
{
	msg->type = XRDP_SERVER_SHARED_FRAMEBUFFER;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_reset(xrdpModule* mod, XRDP_MSG_RESET* msg)
{
	msg->type = XRDP_SERVER_RESET;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_create_offscreen_surface(xrdpModule* mod, XRDP_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	msg->type = XRDP_SERVER_CREATE_OFFSCREEN_SURFACE;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_switch_offscreen_surface(xrdpModule* mod, XRDP_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	msg->type = XRDP_SERVER_SWITCH_OFFSCREEN_SURFACE;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_delete_offscreen_surface(xrdpModule* mod, XRDP_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	msg->type = XRDP_SERVER_DELETE_OFFSCREEN_SURFACE;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_paint_offscreen_surface(xrdpModule* mod, XRDP_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	msg->type = XRDP_SERVER_PAINT_OFFSCREEN_SURFACE;
	return xrdp_server_message_enqueue(mod, (XRDP_MSG_COMMON*) msg);
}

int xrdp_message_server_window_new_update(xrdpModule* mod, XRDP_MSG_WINDOW_NEW_UPDATE* msg)
{
	//mod->ServerProxy->WindowNewUpdate(mod, msg);
	return 0;
}

int xrdp_message_server_window_delete(xrdpModule* mod, XRDP_MSG_WINDOW_DELETE* msg)
{
	//mod->ServerProxy->WindowDelete(mod, msg);
	return 0;
}

int xrdp_message_server_window_icon(xrdpModule* mod, int window_id, int cache_entry, int cache_id, xrdpRailIconInfo* icon_info, int flags)
{
	//mod->ServerProxy->WindowIcon(mod, window_id, cache_entry, cache_id, icon_info, flags);
	return 0;
}

int xrdp_message_server_window_cached_icon(xrdpModule* mod, int window_id, int cache_entry, int cache_id, int flags)
{
	//mod->ServerProxy->WindowCachedIcon(mod, window_id, cache_entry, cache_id, flags);
	return 0;
}

int xrdp_message_server_notify_new_update(xrdpModule* mod, int window_id, int notify_id,
		xrdpRailNotifyStateOrder* notify_state, int flags)
{
	//mod->ServerProxy->NotifyNewUpdate(mod, window_id, notify_id, notify_state, flags);
	return 0;
}

int xrdp_message_server_notify_delete(xrdpModule* mod, int window_id, int notify_id)
{
	//mod->ServerProxy->NotifyDelete(mod, window_id, notify_id);
	return 0;
}

int xrdp_message_server_monitored_desktop(xrdpModule* mod, xrdpRailMonitoredDesktopOrder* mdo, int flags)
{
	//mod->ServerProxy->MonitoredDesktop(mod, mdo, flags);
	return 0;
}

int xrdp_message_server_queue_process_message(xrdpModule* mod, wMessage* message)
{
	int status;

	if (message->id == WMQ_QUIT)
		return 0;

	switch (message->id)
	{
		case XRDP_SERVER_BEGIN_UPDATE:
			status = mod->ServerProxy->BeginUpdate(mod, (XRDP_MSG_BEGIN_UPDATE*) message->wParam);
			break;

		case XRDP_SERVER_END_UPDATE:
			status = mod->ServerProxy->EndUpdate(mod, (XRDP_MSG_END_UPDATE*) message->wParam);
			break;

		case XRDP_SERVER_BEEP:
			status = mod->ServerProxy->Beep(mod, (XRDP_MSG_BEEP*) message->wParam);
			break;

		case XRDP_SERVER_OPAQUE_RECT:
			status = mod->ServerProxy->OpaqueRect(mod, (XRDP_MSG_OPAQUE_RECT*) message->wParam);
			break;

		case XRDP_SERVER_SCREEN_BLT:
			status = mod->ServerProxy->ScreenBlt(mod, (XRDP_MSG_SCREEN_BLT*) message->wParam);
			break;

		case XRDP_SERVER_PAINT_RECT:
			status = mod->ServerProxy->PaintRect(mod, (XRDP_MSG_PAINT_RECT*) message->wParam);
			break;

		case XRDP_SERVER_PATBLT:
			status = mod->ServerProxy->PatBlt(mod, (XRDP_MSG_PATBLT*) message->wParam);
			break;

		case XRDP_SERVER_DSTBLT:
			status = mod->ServerProxy->DstBlt(mod, (XRDP_MSG_DSTBLT*) message->wParam);
			break;

		case XRDP_SERVER_SET_POINTER:
			status = mod->ServerProxy->SetPointer(mod, (XRDP_MSG_SET_POINTER*) message->wParam);
			break;

		case XRDP_SERVER_SET_PALETTE:
			status = mod->ServerProxy->SetPalette(mod, (XRDP_MSG_SET_PALETTE*) message->wParam);
			break;

		case XRDP_SERVER_SET_CLIPPING_REGION:
			status = mod->ServerProxy->SetClippingRegion(mod, (XRDP_MSG_SET_CLIPPING_REGION*) message->wParam);
			break;

		case XRDP_SERVER_LINE_TO:
			status = mod->ServerProxy->LineTo(mod, (XRDP_MSG_LINE_TO*) message->wParam);
			break;

		case XRDP_SERVER_CACHE_GLYPH:
			status = mod->ServerProxy->CacheGlyph(mod, (XRDP_MSG_CACHE_GLYPH*) message->wParam);
			break;

		case XRDP_SERVER_GLYPH_INDEX:
			status = mod->ServerProxy->GlyphIndex(mod, (XRDP_MSG_GLYPH_INDEX*) message->wParam);
			break;

		case XRDP_SERVER_SHARED_FRAMEBUFFER:
			status = mod->ServerProxy->SharedFramebuffer(mod, (XRDP_MSG_SHARED_FRAMEBUFFER*) message->wParam);
			break;

		case XRDP_SERVER_RESET:
			status = mod->ServerProxy->Reset(mod, (XRDP_MSG_RESET*) message->wParam);
			break;

		case XRDP_SERVER_CREATE_OFFSCREEN_SURFACE:
			status = mod->ServerProxy->CreateOffscreenSurface(mod, (XRDP_MSG_CREATE_OFFSCREEN_SURFACE*) message->wParam);
			break;

		case XRDP_SERVER_SWITCH_OFFSCREEN_SURFACE:
			status = mod->ServerProxy->SwitchOffscreenSurface(mod, (XRDP_MSG_SWITCH_OFFSCREEN_SURFACE*) message->wParam);
			break;

		case XRDP_SERVER_DELETE_OFFSCREEN_SURFACE:
			status = mod->ServerProxy->DeleteOffscreenSurface(mod, (XRDP_MSG_DELETE_OFFSCREEN_SURFACE*) message->wParam);
			break;

		case XRDP_SERVER_PAINT_OFFSCREEN_SURFACE:
			status = mod->ServerProxy->PaintOffscreenSurface(mod, (XRDP_MSG_PAINT_OFFSCREEN_SURFACE*) message->wParam);
			break;

		default:
			status = -1;
			break;
	}

	xrdp_server_message_free((XRDP_MSG_COMMON*) message->wParam);

	if (status < 0)
	{
		printf("xrdp_message_server_queue_process_message (%d) status: %d\n", message->id, status);
		return -1;
	}

	return 0;
}

int xrdp_message_server_align_rect(xrdpModule* mod, XRDP_RECT* rect)
{
	if (rect->x < 0)
		rect->x = 0;

	if (rect->x > mod->width - 1)
		rect->x = mod->width - 1;

	rect->width += rect->x % 16;
	rect->x -= rect->x % 16;

	rect->width += rect->width % 16;

	if (rect->x + rect->width > mod->width)
		rect->width = mod->width - rect->x;

	if (rect->y < 0)
		rect->y = 0;

	if (rect->y > mod->height - 1)
		rect->y = mod->height - 1;

	rect->height += rect->y % 16;
	rect->y -= rect->y % 16;

	rect->height += rect->height % 16;

	if (rect->height > mod->height)
		rect->height = mod->height;

	if (rect->y + rect->height > mod->height)
		rect->height = mod->height - rect->y;

	return 0;
}

int xrdp_message_server_queue_pack(xrdpModule* mod)
{
	XRDP_RECT rect;
	wLinkedList* list;
	XRDP_MSG_COMMON* node;
	pixman_bool_t status;
	pixman_box32_t* extents;
	pixman_region32_t region;

	list = mod->ServerList;

	pixman_region32_init(&region);

	LinkedList_Enumerator_Reset(list);

	while (LinkedList_Enumerator_MoveNext(list))
	{
		node = (XRDP_MSG_COMMON*) LinkedList_Enumerator_Current(list);

		if (node->msgFlags & XRDP_MSG_FLAG_RECT)
		{
			status = pixman_region32_union_rect(&region, &region,
					node->rect.x, node->rect.y, node->rect.width, node->rect.height);
		}
		else
		{
			MessageQueue_Post(mod->ServerQueue, (void*) mod, node->type, (void*) node, NULL);
		}
	}

	LinkedList_Clear(list);

	extents = pixman_region32_extents(&region);

	rect.x = extents->x1;
	rect.y = extents->y1;
	rect.width = extents->x2 - extents->x1;
	rect.height = extents->y2 - extents->y1;

	xrdp_message_server_align_rect(mod, &rect);

	if (mod->framebuffer.fbAttached && (rect.width * rect.height))
	{
		XRDP_MSG_COMMON* msg;
		XRDP_MSG_PAINT_RECT paintRect;

		paintRect.type = XRDP_SERVER_PAINT_RECT;

		paintRect.nXSrc = 0;
		paintRect.nYSrc = 0;
		paintRect.bitmapData = NULL;
		paintRect.bitmapDataLength = 0;
		paintRect.framebuffer = &(mod->framebuffer);
		paintRect.fbSegmentId = mod->framebuffer.fbSegmentId;

		paintRect.nLeftRect = rect.x;
		paintRect.nTopRect = rect.y;
		paintRect.nWidth = rect.width;
		paintRect.nHeight = rect.height;

		msg = xrdp_server_message_copy((XRDP_MSG_COMMON*) &paintRect);

		MessageQueue_Post(mod->ServerQueue, (void*) mod, msg->type, (void*) msg, NULL);
	}

	pixman_region32_fini(&region);

	return 0;
}

int xrdp_message_server_queue_process_pending_messages(xrdpModule* mod)
{
	int count;
	int status;
	wMessage message;
	wMessageQueue* queue;

	count = 0;
	status = 0;
	queue = mod->ServerQueue;

	while (MessageQueue_Peek(queue, &message, TRUE))
	{
		status = xrdp_message_server_queue_process_message(mod, &message);

		if (!status)
			break;

		count++;
	}

	return status;
}

int xrdp_message_server_module_init(xrdpModule* mod)
{
	LARGE_INTEGER due;

	mod->ServerProxy = (xrdpServerModule*) malloc(sizeof(xrdpServerModule));

	//mod->ServerProxy = NULL; /* disable */

	if (mod->ServerProxy)
	{
		CopyMemory(mod->ServerProxy, mod->server, sizeof(xrdpServerModule));

		mod->server->BeginUpdate = xrdp_message_server_begin_update;
		mod->server->EndUpdate = xrdp_message_server_end_update;
		mod->server->Beep = xrdp_message_server_beep;
		mod->server->IsTerminated = xrdp_message_server_is_terminated;
		mod->server->OpaqueRect = xrdp_message_server_opaque_rect;
		mod->server->ScreenBlt = xrdp_message_server_screen_blt;
		mod->server->PaintRect = xrdp_message_server_paint_rect;
		mod->server->PatBlt = xrdp_message_server_patblt;
		mod->server->DstBlt = xrdp_message_server_dstblt;
		mod->server->SetPointer = xrdp_message_server_set_pointer;
		mod->server->SetPalette = xrdp_message_server_set_palette;
		mod->server->SetClippingRegion = xrdp_message_server_set_clipping_region;
		mod->server->LineTo = xrdp_message_server_line_to;
		mod->server->CacheGlyph = xrdp_message_server_cache_glyph;
		mod->server->GlyphIndex = xrdp_message_glyph_index;
		mod->server->SharedFramebuffer = xrdp_message_server_shared_framebuffer;
		mod->server->Reset = xrdp_message_server_reset;
		mod->server->CreateOffscreenSurface = xrdp_message_server_create_offscreen_surface;
		mod->server->SwitchOffscreenSurface = xrdp_message_server_switch_offscreen_surface;
		mod->server->DeleteOffscreenSurface = xrdp_message_server_delete_offscreen_surface;
		mod->server->PaintOffscreenSurface = xrdp_message_server_paint_offscreen_surface;
		mod->server->WindowNewUpdate = xrdp_message_server_window_new_update;
		mod->server->WindowDelete = xrdp_message_server_window_delete;
		mod->server->WindowIcon = xrdp_message_server_window_icon;
		mod->server->WindowCachedIcon = xrdp_message_server_window_cached_icon;
		mod->server->NotifyNewUpdate = xrdp_message_server_notify_new_update;
		mod->server->NotifyDelete = xrdp_message_server_notify_delete;
		mod->server->MonitoredDesktop = xrdp_message_server_monitored_desktop;
	}

	mod->ServerList = LinkedList_New();
	mod->ServerQueue = MessageQueue_New();

	due.QuadPart = 0;
	mod->ServerTimer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(mod->ServerTimer, &due, 1000 / 25, NULL, NULL, 0);

	return 0;
}
