/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * xrdp-ng interprocess communication protocol
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

#include <freerds/freerds.h>

#include "transport.h"

#include "outbound.h"

int freerds_client_outbound_synchronize_keyboard_event(rdsModule* mod, DWORD flags)
{
	int length;
	int status;
	wStream* s;
	RDS_MSG_SYNCHRONIZE_KEYBOARD_EVENT msg;

	msg.msgFlags = 0;
	msg.type = RDS_CLIENT_SYNCHRONIZE_KEYBOARD_EVENT;

	msg.flags = flags;

	s = mod->OutboundStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_synchronize_keyboard_event(NULL, &msg);
	xrdp_write_synchronize_keyboard_event(s, &msg);

	status = freerds_named_pipe_write(mod->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_scancode_keyboard_event(rdsModule* mod, DWORD flags, DWORD code, DWORD keyboardType)
{
	int length;
	int status;
	wStream* s;
	RDS_MSG_SCANCODE_KEYBOARD_EVENT msg;

	msg.msgFlags = 0;
	msg.type = RDS_CLIENT_SCANCODE_KEYBOARD_EVENT;

	msg.flags = flags;
	msg.code = code;
	msg.keyboardType = keyboardType;

	s = mod->OutboundStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_scancode_keyboard_event(NULL, &msg);
	xrdp_write_scancode_keyboard_event(s, &msg);

	status = freerds_named_pipe_write(mod->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_virtual_keyboard_event(rdsModule* mod, DWORD flags, DWORD code)
{
	int length;
	int status;
	wStream* s;
	RDS_MSG_VIRTUAL_KEYBOARD_EVENT msg;

	msg.msgFlags = 0;
	msg.type = RDS_CLIENT_VIRTUAL_KEYBOARD_EVENT;

	msg.flags = flags;
	msg.code = code;

	s = mod->OutboundStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_virtual_keyboard_event(NULL, &msg);
	xrdp_write_virtual_keyboard_event(s, &msg);

	status = freerds_named_pipe_write(mod->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_unicode_keyboard_event(rdsModule* mod, DWORD flags, DWORD code)
{
	int length;
	int status;
	wStream* s;
	RDS_MSG_UNICODE_KEYBOARD_EVENT msg;

	msg.msgFlags = 0;
	msg.type = RDS_CLIENT_UNICODE_KEYBOARD_EVENT;

	msg.flags = flags;
	msg.code = code;

	s = mod->OutboundStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_unicode_keyboard_event(NULL, &msg);
	xrdp_write_unicode_keyboard_event(s, &msg);

	status = freerds_named_pipe_write(mod->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_mouse_event(rdsModule* mod, DWORD flags, DWORD x, DWORD y)
{
	int length;
	int status;
	wStream* s;
	RDS_MSG_MOUSE_EVENT msg;

	msg.msgFlags = 0;
	msg.type = RDS_CLIENT_MOUSE_EVENT;

	msg.flags = flags;
	msg.x = x;
	msg.y = y;

	s = mod->OutboundStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_mouse_event(NULL, &msg);
	xrdp_write_mouse_event(s, &msg);

	status = freerds_named_pipe_write(mod->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_extended_mouse_event(rdsModule* mod, DWORD flags, DWORD x, DWORD y)
{
	int length;
	int status;
	wStream* s;
	RDS_MSG_EXTENDED_MOUSE_EVENT msg;

	msg.msgFlags = 0;
	msg.type = RDS_CLIENT_EXTENDED_MOUSE_EVENT;

	msg.flags = flags;
	msg.x = x;
	msg.y = y;

	s = mod->OutboundStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_extended_mouse_event(NULL, &msg);
	xrdp_write_extended_mouse_event(s, &msg);

	status = freerds_named_pipe_write(mod->hClientPipe, Stream_Buffer(s), length);

	return status;
}

rdsClientInterface* freerds_client_outbound_interface_new()
{
	rdsClientInterface* client;

	client = (rdsClientInterface*) malloc(sizeof(rdsClientInterface));

	if (client)
	{
		ZeroMemory(client, sizeof(rdsClientInterface));

		client->SynchronizeKeyboardEvent = freerds_client_outbound_synchronize_keyboard_event;
		client->ScancodeKeyboardEvent = freerds_client_outbound_scancode_keyboard_event;
		client->VirtualKeyboardEvent = freerds_client_outbound_virtual_keyboard_event;
		client->UnicodeKeyboardEvent = freerds_client_outbound_unicode_keyboard_event;
		client->MouseEvent = freerds_client_outbound_mouse_event;
		client->ExtendedMouseEvent = freerds_client_outbound_extended_mouse_event;
	}

	return client;
}

int freerds_server_outbound_write_message(rdsModule* mod, RDS_MSG_COMMON* msg)
{
	int status;
	wStream* s;

	s = mod->OutboundStream;
	Stream_SetPosition(s, 0);

	xrdp_server_message_write(NULL, msg);
	Stream_EnsureRemainingCapacity(s, msg->length);
	xrdp_server_message_write(s, msg);

	status = freerds_named_pipe_write(mod->hClientPipe, Stream_Buffer(s), msg->length);

	return status;
}

int freerds_server_outbound_begin_update(rdsModule* mod, RDS_MSG_BEGIN_UPDATE* msg)
{
	msg->type = RDS_SERVER_BEGIN_UPDATE;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_end_update(rdsModule* mod, RDS_MSG_END_UPDATE* msg)
{
	msg->type = RDS_SERVER_END_UPDATE;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_beep(rdsModule* mod, RDS_MSG_BEEP* msg)
{
	msg->type = RDS_SERVER_BEEP;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_is_terminated(rdsModule* mod)
{
	return 0;
}

int freerds_server_outbound_opaque_rect(rdsModule* mod, RDS_MSG_OPAQUE_RECT* msg)
{
	msg->type = RDS_SERVER_OPAQUE_RECT;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_screen_blt(rdsModule* mod, RDS_MSG_SCREEN_BLT* msg)
{
	msg->type = RDS_SERVER_SCREEN_BLT;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_paint_rect(rdsModule* mod, RDS_MSG_PAINT_RECT* msg)
{
	msg->type = RDS_SERVER_PAINT_RECT;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_patblt(rdsModule* mod, RDS_MSG_PATBLT* msg)
{
	msg->type = RDS_SERVER_PATBLT;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_dstblt(rdsModule* mod, RDS_MSG_DSTBLT* msg)
{
	msg->type = RDS_SERVER_DSTBLT;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_set_pointer(rdsModule* mod, RDS_MSG_SET_POINTER* msg)
{
	msg->type = RDS_SERVER_SET_POINTER;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_set_palette(rdsModule* mod, RDS_MSG_SET_PALETTE* msg)
{
	msg->type = RDS_SERVER_SET_PALETTE;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_set_clipping_region(rdsModule* mod, RDS_MSG_SET_CLIPPING_REGION* msg)
{
	msg->type = RDS_SERVER_SET_CLIPPING_REGION;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_line_to(rdsModule* mod, RDS_MSG_LINE_TO* msg)
{
	msg->type = RDS_SERVER_LINE_TO;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_cache_glyph(rdsModule* mod, RDS_MSG_CACHE_GLYPH* msg)
{
	msg->type = RDS_SERVER_CACHE_GLYPH;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_glyph_index(rdsModule* mod, RDS_MSG_GLYPH_INDEX* msg)
{
	msg->type = RDS_SERVER_GLYPH_INDEX;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_shared_framebuffer(rdsModule* mod, RDS_MSG_SHARED_FRAMEBUFFER* msg)
{
	msg->type = RDS_SERVER_SHARED_FRAMEBUFFER;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_reset(rdsModule* mod, RDS_MSG_RESET* msg)
{
	msg->type = RDS_SERVER_RESET;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_create_offscreen_surface(rdsModule* mod, RDS_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_CREATE_OFFSCREEN_SURFACE;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_switch_offscreen_surface(rdsModule* mod, RDS_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_SWITCH_OFFSCREEN_SURFACE;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_delete_offscreen_surface(rdsModule* mod, RDS_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_DELETE_OFFSCREEN_SURFACE;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_paint_offscreen_surface(rdsModule* mod, RDS_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_PAINT_OFFSCREEN_SURFACE;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_window_new_update(rdsModule* mod, RDS_MSG_WINDOW_NEW_UPDATE* msg)
{
	msg->type = RDS_SERVER_WINDOW_NEW_UPDATE;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_window_delete(rdsModule* mod, RDS_MSG_WINDOW_DELETE* msg)
{
	msg->type = RDS_SERVER_WINDOW_DELETE;
	return freerds_server_outbound_write_message(mod, (RDS_MSG_COMMON*) msg);
}

rdsServerInterface* freerds_server_outbound_interface_new()
{
	rdsServerInterface* server;

	server = (rdsServerInterface*) malloc(sizeof(rdsServerInterface));

	if (server)
	{
		ZeroMemory(server, sizeof(rdsServerInterface));

		server->BeginUpdate = freerds_server_outbound_begin_update;
		server->EndUpdate = freerds_server_outbound_end_update;
		server->Beep = freerds_server_outbound_beep;
		server->IsTerminated = freerds_server_outbound_is_terminated;
		server->OpaqueRect = freerds_server_outbound_opaque_rect;
		server->ScreenBlt = freerds_server_outbound_screen_blt;
		server->PaintRect = freerds_server_outbound_paint_rect;
		server->PatBlt = freerds_server_outbound_patblt;
		server->DstBlt = freerds_server_outbound_dstblt;
		server->SetPointer = freerds_server_outbound_set_pointer;
		server->SetPalette = freerds_server_outbound_set_palette;
		server->SetClippingRegion = freerds_server_outbound_set_clipping_region;
		server->LineTo = freerds_server_outbound_line_to;
		server->CacheGlyph = freerds_server_outbound_cache_glyph;
		server->GlyphIndex = freerds_server_outbound_glyph_index;
		server->SharedFramebuffer = freerds_server_outbound_shared_framebuffer;
		server->Reset = freerds_server_outbound_reset;
		server->CreateOffscreenSurface = freerds_server_outbound_create_offscreen_surface;
		server->SwitchOffscreenSurface = freerds_server_outbound_switch_offscreen_surface;
		server->DeleteOffscreenSurface = freerds_server_outbound_delete_offscreen_surface;
		server->PaintOffscreenSurface = freerds_server_outbound_paint_offscreen_surface;
		server->WindowNewUpdate = freerds_server_outbound_window_new_update;
		server->WindowDelete = freerds_server_outbound_window_delete;
	}

	return server;
}
