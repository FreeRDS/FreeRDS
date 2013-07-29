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

int xrdp_message_server_begin_update(xrdpModule* mod)
{
	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_BEGIN_UPDATE, NULL, NULL);
	return 0;
}

int xrdp_message_server_end_update(xrdpModule* mod)
{
	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_END_UPDATE, NULL, NULL);
	return 0;
}

int xrdp_message_server_beep(xrdpModule* mod)
{
	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_BEEP, NULL, NULL);
	return 0;
}

int xrdp_message_server_message(xrdpModule* mod, char* msg, int code)
{
	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_MESSAGE, (void*) msg, (void*) (size_t) code);
	return 0;
}

int xrdp_message_server_is_terminated(xrdpModule* mod)
{
	int status;
	status = mod->ServerProxy->IsTerminated(mod);
	return status;
}

int xrdp_message_server_opaque_rect(xrdpModule* mod, XRDP_MSG_OPAQUE_RECT* msg)
{
	XRDP_MSG_OPAQUE_RECT* wParam;

	wParam = (XRDP_MSG_OPAQUE_RECT*) malloc(sizeof(XRDP_MSG_OPAQUE_RECT));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_OPAQUE_RECT));

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_OPAQUE_RECT, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_screen_blt(xrdpModule* mod, XRDP_MSG_SCREEN_BLT* msg)
{
	XRDP_MSG_SCREEN_BLT* wParam;

	wParam = (XRDP_MSG_SCREEN_BLT*) malloc(sizeof(XRDP_MSG_SCREEN_BLT));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_SCREEN_BLT));

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_SCREEN_BLT, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_paint_rect(xrdpModule* mod, XRDP_MSG_PAINT_RECT* msg)
{
	XRDP_MSG_PAINT_RECT* wParam;

	wParam = (XRDP_MSG_PAINT_RECT*) malloc(sizeof(XRDP_MSG_PAINT_RECT));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_PAINT_RECT));

	if (msg->bitmapDataLength)
	{
		wParam->bitmapData = (BYTE*) malloc(msg->bitmapDataLength);
		CopyMemory(wParam->bitmapData, msg->bitmapData, msg->bitmapDataLength);
	}

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_PAINT_RECT, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_patblt(xrdpModule* mod, XRDP_MSG_PATBLT* msg)
{
	XRDP_MSG_PATBLT* wParam;

	wParam = (XRDP_MSG_PATBLT*) malloc(sizeof(XRDP_MSG_PATBLT));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_PATBLT));

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_PATBLT, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_dstblt(xrdpModule* mod, XRDP_MSG_DSTBLT* msg)
{
	XRDP_MSG_DSTBLT* wParam;

	wParam = (XRDP_MSG_DSTBLT*) malloc(sizeof(XRDP_MSG_DSTBLT));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_DSTBLT));

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_DSTBLT, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_set_pointer(xrdpModule* mod, XRDP_MSG_SET_POINTER* msg)
{
	XRDP_MSG_SET_POINTER* wParam;

	wParam = (XRDP_MSG_SET_POINTER*) malloc(sizeof(XRDP_MSG_SET_POINTER));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_SET_POINTER));

	if (wParam->andMaskData)
	{
		wParam->andMaskData = (BYTE*) malloc(wParam->lengthAndMask);
		CopyMemory(wParam->andMaskData, msg->andMaskData, wParam->lengthAndMask);
	}

	if (wParam->xorMaskData)
	{
		wParam->xorMaskData = (BYTE*) malloc(wParam->lengthXorMask);
		CopyMemory(wParam->xorMaskData, msg->xorMaskData, wParam->lengthXorMask);
	}

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_SET_POINTER, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_set_palette(xrdpModule* mod, XRDP_MSG_SET_PALETTE* msg)
{
	XRDP_MSG_SET_PALETTE* wParam;

	wParam = (XRDP_MSG_SET_PALETTE*) malloc(sizeof(XRDP_MSG_SET_PALETTE));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_SET_PALETTE));

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_SET_PALETTE, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_set_clipping_region(xrdpModule* mod, XRDP_MSG_SET_CLIPPING_REGION* msg)
{
	XRDP_MSG_SET_CLIPPING_REGION* wParam;

	wParam = (XRDP_MSG_SET_CLIPPING_REGION*) malloc(sizeof(XRDP_MSG_SET_CLIPPING_REGION));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_SET_CLIPPING_REGION));

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_SET_CLIPPING_REGION, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_set_null_clipping_region(xrdpModule* mod)
{
	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_SET_NULL_CLIPPING_REGION, NULL, NULL);
	return 0;
}

int xrdp_message_server_set_rop2(xrdpModule* mod, int opcode)
{
	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_SET_ROP2, (void*) (size_t) opcode, NULL);
	return 0;
}

int xrdp_message_server_line_to(xrdpModule* mod, XRDP_MSG_LINE_TO* msg)
{
	XRDP_MSG_LINE_TO* wParam;

	wParam = (XRDP_MSG_LINE_TO*) malloc(sizeof(XRDP_MSG_LINE_TO));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_LINE_TO));

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_LINE_TO, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_add_char(xrdpModule* mod, XRDP_MSG_CACHE_GLYPH* msg)
{
	XRDP_MSG_CACHE_GLYPH* wParam;

	wParam = (XRDP_MSG_CACHE_GLYPH*) malloc(sizeof(XRDP_MSG_CACHE_GLYPH));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_CACHE_GLYPH));

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_CACHE_GLYPH, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_text(xrdpModule* mod, XRDP_MSG_GLYPH_INDEX* msg)
{
	XRDP_MSG_GLYPH_INDEX* wParam;

	wParam = (XRDP_MSG_GLYPH_INDEX*) malloc(sizeof(XRDP_MSG_GLYPH_INDEX));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_GLYPH_INDEX));

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_GLYPH_INDEX, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_shared_framebuffer(xrdpModule* mod, XRDP_MSG_SHARED_FRAMEBUFFER* msg)
{
	XRDP_MSG_SHARED_FRAMEBUFFER* wParam;

	wParam = (XRDP_MSG_SHARED_FRAMEBUFFER*) malloc(sizeof(XRDP_MSG_SHARED_FRAMEBUFFER));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_SHARED_FRAMEBUFFER));

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_SHARED_FRAMEBUFFER, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_reset(xrdpModule* mod, XRDP_MSG_RESET* msg)
{
	XRDP_MSG_RESET* wParam;

	wParam = (XRDP_MSG_RESET*) malloc(sizeof(XRDP_MSG_RESET));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_RESET));

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_RESET, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_create_offscreen_surface(xrdpModule* mod, XRDP_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	XRDP_MSG_CREATE_OFFSCREEN_SURFACE* wParam;

	wParam = (XRDP_MSG_CREATE_OFFSCREEN_SURFACE*) malloc(sizeof(XRDP_MSG_CREATE_OFFSCREEN_SURFACE));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_CREATE_OFFSCREEN_SURFACE));

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_CREATE_OFFSCREEN_SURFACE, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_switch_offscreen_surface(xrdpModule* mod, XRDP_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	XRDP_MSG_SWITCH_OFFSCREEN_SURFACE* wParam;

	wParam = (XRDP_MSG_SWITCH_OFFSCREEN_SURFACE*) malloc(sizeof(XRDP_MSG_SWITCH_OFFSCREEN_SURFACE));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_SWITCH_OFFSCREEN_SURFACE));

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_SWITCH_OFFSCREEN_SURFACE, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_delete_offscreen_surface(xrdpModule* mod, XRDP_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	XRDP_MSG_DELETE_OFFSCREEN_SURFACE* wParam;

	wParam = (XRDP_MSG_DELETE_OFFSCREEN_SURFACE*) malloc(sizeof(XRDP_MSG_DELETE_OFFSCREEN_SURFACE));
	CopyMemory(wParam, msg, sizeof(XRDP_MSG_DELETE_OFFSCREEN_SURFACE));

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_DELETE_OFFSCREEN_SURFACE, (void*) wParam, NULL);

	return 0;
}

int xrdp_message_server_paint_offscreen_rect(xrdpModule* mod, int x, int y, int cx, int cy, int cacheIndex, int srcx, int srcy)
{
	XRDP_MSG_MEMBLT* wParam;

	wParam = (XRDP_MSG_MEMBLT*) malloc(sizeof(XRDP_MSG_MEMBLT));

	wParam->nLeftRect = x;
	wParam->nTopRect = y;
	wParam->nWidth = cx;
	wParam->nHeight = cy;
	wParam->index = cacheIndex;
	wParam->nXSrc = srcx;
	wParam->nYSrc = srcy;

	MessageQueue_Post(mod->ServerQueue, (void*) mod, XRDP_SERVER_MEMBLT, (void*) wParam, NULL);

	return 0;
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
			status = mod->ServerProxy->BeginUpdate(mod);
			break;

		case XRDP_SERVER_END_UPDATE:
			status = mod->ServerProxy->EndUpdate(mod);
			break;

		case XRDP_SERVER_BEEP:
			status = mod->ServerProxy->Beep(mod);
			break;

		case XRDP_SERVER_MESSAGE:
			status = mod->ServerProxy->Message(mod, (char*) message->wParam, (int) (size_t) message->lParam);
			break;

		case XRDP_SERVER_OPAQUE_RECT:
			status = mod->ServerProxy->OpaqueRect(mod, (XRDP_MSG_OPAQUE_RECT*) message->wParam);
			free(message->wParam);
			break;

		case XRDP_SERVER_SCREEN_BLT:
			status = mod->ServerProxy->ScreenBlt(mod, (XRDP_MSG_SCREEN_BLT*) message->wParam);
			free(message->wParam);
			break;

		case XRDP_SERVER_PAINT_RECT:
			{
				XRDP_MSG_PAINT_RECT* wParam = (XRDP_MSG_PAINT_RECT*) message->wParam;
				status = mod->ServerProxy->PaintRect(mod, wParam);

				if (wParam->bitmapDataLength)
					free(wParam->bitmapData);

				free(message->wParam);
			}
			break;

		case XRDP_SERVER_PATBLT:
			status = mod->ServerProxy->PatBlt(mod, (XRDP_MSG_PATBLT*) message->wParam);
			free(message->wParam);
			break;

		case XRDP_SERVER_DSTBLT:
			status = mod->ServerProxy->DstBlt(mod, (XRDP_MSG_DSTBLT*) message->wParam);
			free(message->wParam);
			break;

		case XRDP_SERVER_SET_POINTER:
			{
				XRDP_MSG_SET_POINTER* wParam = (XRDP_MSG_SET_POINTER*) message->wParam;
				status = mod->ServerProxy->SetPointer(mod, wParam);

				if (wParam->andMaskData)
					free(wParam->andMaskData);

				if (wParam->xorMaskData)
					free(wParam->xorMaskData);

				free(wParam);
			}
			break;

		case XRDP_SERVER_SET_PALETTE:
			status = mod->ServerProxy->SetPalette(mod, (XRDP_MSG_SET_PALETTE*) message->wParam);
			free(message->wParam);
			break;

		case XRDP_SERVER_SET_CLIPPING_REGION:
			status = mod->ServerProxy->SetClippingRegion(mod, (XRDP_MSG_SET_CLIPPING_REGION*) message->wParam);
			free(message->wParam);
			break;

		case XRDP_SERVER_SET_NULL_CLIPPING_REGION:
			status = mod->ServerProxy->SetNullClippingRegion(mod);
			break;

		case XRDP_SERVER_SET_ROP2:
			status = mod->ServerProxy->SetRop2(mod, (int) (size_t) message->wParam);
			break;

		case XRDP_SERVER_LINE_TO:
			status = mod->ServerProxy->LineTo(mod, (XRDP_MSG_LINE_TO*) message->wParam);
			free(message->wParam);
			break;

		case XRDP_SERVER_CACHE_GLYPH:
			status = mod->ServerProxy->AddChar(mod, (XRDP_MSG_CACHE_GLYPH*) message->wParam);
			free(message->wParam);
			break;

		case XRDP_SERVER_GLYPH_INDEX:
			status = mod->ServerProxy->Text(mod, (XRDP_MSG_GLYPH_INDEX*) message->wParam);
			free(message->wParam);
			break;

		case XRDP_SERVER_SHARED_FRAMEBUFFER:
			status = mod->ServerProxy->SharedFramebuffer(mod, (XRDP_MSG_SHARED_FRAMEBUFFER*) message->wParam);
			free(message->wParam);
			break;

		case XRDP_SERVER_RESET:
			status = mod->ServerProxy->Reset(mod, (XRDP_MSG_RESET*) message->wParam);
			free(message->wParam);
			break;

		case XRDP_SERVER_CREATE_OFFSCREEN_SURFACE:
			{
				XRDP_MSG_CREATE_OFFSCREEN_SURFACE* wParam = (XRDP_MSG_CREATE_OFFSCREEN_SURFACE*) message->wParam;
				status = mod->ServerProxy->CreateOffscreenSurface(mod, wParam);
				free(wParam);
			}
			break;

		case XRDP_SERVER_SWITCH_OFFSCREEN_SURFACE:
			{
				XRDP_MSG_SWITCH_OFFSCREEN_SURFACE* wParam = (XRDP_MSG_SWITCH_OFFSCREEN_SURFACE*) message->wParam;
				status = mod->ServerProxy->SwitchOffscreenSurface(mod, wParam);
				free(wParam);
			}
			break;

		case XRDP_SERVER_DELETE_OFFSCREEN_SURFACE:
			{
				XRDP_MSG_DELETE_OFFSCREEN_SURFACE* wParam = (XRDP_MSG_DELETE_OFFSCREEN_SURFACE*) message->wParam;
				status = mod->ServerProxy->DeleteOffscreenSurface(mod, wParam);
				free(wParam);
			}
			break;

		case XRDP_SERVER_MEMBLT:
			{
				XRDP_MSG_MEMBLT* wParam = (XRDP_MSG_MEMBLT*) message->wParam;

				status = mod->ServerProxy->PaintOffscreenRect(mod, wParam->nLeftRect, wParam->nTopRect,
						wParam->nWidth, wParam->nHeight, wParam->index, wParam->nXSrc, wParam->nYSrc);

				free(wParam);
			}
			break;

		default:
			status = -1;
			break;
	}

	if (status < 0)
	{
		printf("xrdp_message_server_queue_process_message (%d) status: %d\n", message->id, status);
		return -1;
	}

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
	mod->ServerProxy = (xrdpServerModule*) malloc(sizeof(xrdpServerModule));

	//mod->ServerProxy = NULL; /* disable */

	if (mod->ServerProxy)
	{
		CopyMemory(mod->ServerProxy, mod->server, sizeof(xrdpServerModule));

		mod->server->BeginUpdate = xrdp_message_server_begin_update;
		mod->server->EndUpdate = xrdp_message_server_end_update;
		mod->server->Beep = xrdp_message_server_beep;
		mod->server->Message = xrdp_message_server_message;
		mod->server->IsTerminated = xrdp_message_server_is_terminated;
		mod->server->OpaqueRect = xrdp_message_server_opaque_rect;
		mod->server->ScreenBlt = xrdp_message_server_screen_blt;
		mod->server->PaintRect = xrdp_message_server_paint_rect;
		mod->server->PatBlt = xrdp_message_server_patblt;
		mod->server->DstBlt = xrdp_message_server_dstblt;
		mod->server->SetPointer = xrdp_message_server_set_pointer;
		mod->server->SetPalette = xrdp_message_server_set_palette;
		mod->server->SetClippingRegion = xrdp_message_server_set_clipping_region;
		mod->server->SetNullClippingRegion = xrdp_message_server_set_null_clipping_region;
		mod->server->SetRop2 = xrdp_message_server_set_rop2;
		mod->server->LineTo = xrdp_message_server_line_to;
		mod->server->AddChar = xrdp_message_server_add_char;
		mod->server->Text = xrdp_message_server_text;
		mod->server->SharedFramebuffer = xrdp_message_server_shared_framebuffer;
		mod->server->Reset = xrdp_message_server_reset;
		mod->server->CreateOffscreenSurface = xrdp_message_server_create_offscreen_surface;
		mod->server->SwitchOffscreenSurface = xrdp_message_server_switch_offscreen_surface;
		mod->server->DeleteOffscreenSurface = xrdp_message_server_delete_offscreen_surface;
		mod->server->PaintOffscreenRect = xrdp_message_server_paint_offscreen_rect;
		mod->server->WindowNewUpdate = xrdp_message_server_window_new_update;
		mod->server->WindowDelete = xrdp_message_server_window_delete;
		mod->server->WindowIcon = xrdp_message_server_window_icon;
		mod->server->WindowCachedIcon = xrdp_message_server_window_cached_icon;
		mod->server->NotifyNewUpdate = xrdp_message_server_notify_new_update;
		mod->server->NotifyDelete = xrdp_message_server_notify_delete;
		mod->server->MonitoredDesktop = xrdp_message_server_monitored_desktop;
	}

	mod->ServerQueue = MessageQueue_New();

	return 0;
}
