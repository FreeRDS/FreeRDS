/**
 * FreeRDS
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

#include "transport.h"

#include "outbound.h"

int freerds_client_outbound_write_message(rdsBackend* backend, RDS_MSG_COMMON* msg)
{
	int status;
	wStream* s;

	s = backend->OutboundStream;
	Stream_SetPosition(s, 0);

	freerds_client_message_write(NULL, msg);
	Stream_EnsureRemainingCapacity(s, msg->length);
	freerds_client_message_write(s, msg);

	status = freerds_named_pipe_write(backend->hClientPipe, Stream_Buffer(s), msg->length);

	return status;
}

int freerds_client_outbound_synchronize_keyboard_event(rdsBackend* backend, DWORD flags)
{
	int length;
	int status;
	wStream* s;
	RDS_MSG_SYNCHRONIZE_KEYBOARD_EVENT msg;

	msg.msgFlags = 0;
	msg.type = RDS_CLIENT_SYNCHRONIZE_KEYBOARD_EVENT;

	msg.flags = flags;

	s = backend->OutboundStream;
	Stream_SetPosition(s, 0);

	length = freerds_write_synchronize_keyboard_event(NULL, &msg);
	freerds_write_synchronize_keyboard_event(s, &msg);

	status = freerds_named_pipe_write(backend->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_scancode_keyboard_event(rdsBackend* backend, DWORD flags, DWORD code, DWORD keyboardType)
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

	s = backend->OutboundStream;
	Stream_SetPosition(s, 0);

	length = freerds_write_scancode_keyboard_event(NULL, &msg);
	freerds_write_scancode_keyboard_event(s, &msg);

	status = freerds_named_pipe_write(backend->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_virtual_keyboard_event(rdsBackend* backend, DWORD flags, DWORD code)
{
	int length;
	int status;
	wStream* s;
	RDS_MSG_VIRTUAL_KEYBOARD_EVENT msg;

	msg.msgFlags = 0;
	msg.type = RDS_CLIENT_VIRTUAL_KEYBOARD_EVENT;

	msg.flags = flags;
	msg.code = code;

	s = backend->OutboundStream;
	Stream_SetPosition(s, 0);

	length = freerds_write_virtual_keyboard_event(NULL, &msg);
	freerds_write_virtual_keyboard_event(s, &msg);

	status = freerds_named_pipe_write(backend->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_unicode_keyboard_event(rdsBackend* backend, DWORD flags, DWORD code)
{
	int length;
	int status;
	wStream* s;
	RDS_MSG_UNICODE_KEYBOARD_EVENT msg;

	msg.msgFlags = 0;
	msg.type = RDS_CLIENT_UNICODE_KEYBOARD_EVENT;

	msg.flags = flags;
	msg.code = code;

	s = backend->OutboundStream;
	Stream_SetPosition(s, 0);

	length = freerds_write_unicode_keyboard_event(NULL, &msg);
	freerds_write_unicode_keyboard_event(s, &msg);

	status = freerds_named_pipe_write(backend->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_mouse_event(rdsBackend* backend, DWORD flags, DWORD x, DWORD y)
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

	s = backend->OutboundStream;
	Stream_SetPosition(s, 0);

	length = freerds_write_mouse_event(NULL, &msg);
	freerds_write_mouse_event(s, &msg);

	status = freerds_named_pipe_write(backend->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_extended_mouse_event(rdsBackend* backend, DWORD flags, DWORD x, DWORD y)
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

	s = backend->OutboundStream;
	Stream_SetPosition(s, 0);

	length = freerds_write_extended_mouse_event(NULL, &msg);
	freerds_write_extended_mouse_event(s, &msg);

	status = freerds_named_pipe_write(backend->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_vblank_event(rdsBackend* backend)
{
	int length;
	int status;
	wStream* s;
	RDS_MSG_VBLANK_EVENT msg;

	msg.msgFlags = 0;
	msg.type = RDS_CLIENT_VBLANK_EVENT;

	s = backend->OutboundStream;
	Stream_SetPosition(s, 0);

	length = freerds_write_vblank_event(NULL, &msg);
	freerds_write_vblank_event(s, &msg);

	status = freerds_named_pipe_write(backend->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_logon_user(rdsBackend* backend, RDS_MSG_LOGON_USER* msg)
{
	msg->type = RDS_CLIENT_LOGON_USER;
	return freerds_client_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_client_outbound_logoff_user(rdsBackend* backend, RDS_MSG_LOGOFF_USER* msg)
{
	msg->type = RDS_CLIENT_LOGOFF_USER;
	return freerds_client_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_client_outbound_suppress_output(rdsBackend* backend, UINT32 suppress_output)
{
	int length;
	int status;
	wStream* s;
	RDS_MSG_SUPPRESS_OUTPUT msg;

	msg.msgFlags = 0;
	msg.type = RDS_CLIENT_SUPPRESS_OUTPUT;
	msg.activeOutput = suppress_output;

	s = backend->OutboundStream;
	Stream_SetPosition(s, 0);

	length = freerds_write_suppress_output(NULL, &msg);
	freerds_write_suppress_output(s, &msg);

	status = freerds_named_pipe_write(backend->hClientPipe, Stream_Buffer(s), length);

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
		client->VBlankEvent = freerds_client_outbound_vblank_event;
		client->LogonUser = freerds_client_outbound_logon_user;
		client->LogoffUser = freerds_client_outbound_logoff_user;
		client->SuppressOutput = freerds_client_outbound_suppress_output;
	}

	return client;
}

int freerds_server_outbound_write_message(rdsBackend* backend, RDS_MSG_COMMON* msg)
{
	int status;
	wStream* s;

	s = backend->OutboundStream;
	Stream_SetPosition(s, 0);

	freerds_server_message_write(NULL, msg);
	Stream_EnsureRemainingCapacity(s, msg->length);
	freerds_server_message_write(s, msg);

	status = freerds_named_pipe_write(backend->hClientPipe, Stream_Buffer(s), msg->length);

	return status;
}

int freerds_server_outbound_begin_update(rdsBackend* backend, RDS_MSG_BEGIN_UPDATE* msg)
{
	msg->type = RDS_SERVER_BEGIN_UPDATE;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_end_update(rdsBackend* backend, RDS_MSG_END_UPDATE* msg)
{
	msg->type = RDS_SERVER_END_UPDATE;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_beep(rdsBackend* backend, RDS_MSG_BEEP* msg)
{
	msg->type = RDS_SERVER_BEEP;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_is_terminated(rdsBackend* backend)
{
	return 0;
}

int freerds_server_outbound_opaque_rect(rdsBackend* backend, RDS_MSG_OPAQUE_RECT* msg)
{
	msg->type = RDS_SERVER_OPAQUE_RECT;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_screen_blt(rdsBackend* backend, RDS_MSG_SCREEN_BLT* msg)
{
	msg->type = RDS_SERVER_SCREEN_BLT;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_paint_rect(rdsBackend* backend, RDS_MSG_PAINT_RECT* msg)
{
	msg->type = RDS_SERVER_PAINT_RECT;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_patblt(rdsBackend* backend, RDS_MSG_PATBLT* msg)
{
	msg->type = RDS_SERVER_PATBLT;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_dstblt(rdsBackend* backend, RDS_MSG_DSTBLT* msg)
{
	msg->type = RDS_SERVER_DSTBLT;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_set_pointer(rdsBackend* backend, RDS_MSG_SET_POINTER* msg)
{
	msg->type = RDS_SERVER_SET_POINTER;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_set_system_pointer(rdsBackend* backend, RDS_MSG_SET_SYSTEM_POINTER* msg)
{
	msg->type = RDS_SERVER_SET_SYSTEM_POINTER;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_set_palette(rdsBackend* backend, RDS_MSG_SET_PALETTE* msg)
{
	msg->type = RDS_SERVER_SET_PALETTE;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_set_clipping_region(rdsBackend* backend, RDS_MSG_SET_CLIPPING_REGION* msg)
{
	msg->type = RDS_SERVER_SET_CLIPPING_REGION;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_line_to(rdsBackend* backend, RDS_MSG_LINE_TO* msg)
{
	msg->type = RDS_SERVER_LINE_TO;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_cache_glyph(rdsBackend* backend, RDS_MSG_CACHE_GLYPH* msg)
{
	msg->type = RDS_SERVER_CACHE_GLYPH;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_glyph_index(rdsBackend* backend, RDS_MSG_GLYPH_INDEX* msg)
{
	msg->type = RDS_SERVER_GLYPH_INDEX;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_shared_framebuffer(rdsBackend* backend, RDS_MSG_SHARED_FRAMEBUFFER* msg)
{
	msg->type = RDS_SERVER_SHARED_FRAMEBUFFER;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_reset(rdsBackend* backend, RDS_MSG_RESET* msg)
{
	msg->type = RDS_SERVER_RESET;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_create_offscreen_surface(rdsBackend* backend, RDS_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_CREATE_OFFSCREEN_SURFACE;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_switch_offscreen_surface(rdsBackend* backend, RDS_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_SWITCH_OFFSCREEN_SURFACE;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_delete_offscreen_surface(rdsBackend* backend, RDS_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_DELETE_OFFSCREEN_SURFACE;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_paint_offscreen_surface(rdsBackend* backend, RDS_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	msg->type = RDS_SERVER_PAINT_OFFSCREEN_SURFACE;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_window_new_update(rdsBackend* backend, RDS_MSG_WINDOW_NEW_UPDATE* msg)
{
	msg->type = RDS_SERVER_WINDOW_NEW_UPDATE;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
}

int freerds_server_outbound_window_delete(rdsBackend* backend, RDS_MSG_WINDOW_DELETE* msg)
{
	msg->type = RDS_SERVER_WINDOW_DELETE;
	return freerds_server_outbound_write_message(backend, (RDS_MSG_COMMON*) msg);
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
		server->SetSystemPointer = freerds_server_outbound_set_system_pointer;
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
