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

#include <xrdp-ng/xrdp.h>

#include "transport.h"

#include "outbound.h"

int freerds_client_outbound_synchronize_keyboard_event(xrdpModule* mod, DWORD flags)
{
	int length;
	int status;
	wStream* s;
	XRDP_MSG_SYNCHRONIZE_KEYBOARD_EVENT msg;

	msg.msgFlags = 0;
	msg.type = XRDP_CLIENT_SYNCHRONIZE_KEYBOARD_EVENT;

	msg.flags = flags;

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_synchronize_keyboard_event(NULL, &msg);
	xrdp_write_synchronize_keyboard_event(s, &msg);

	status = freerds_named_pipe_write(mod->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_scancode_keyboard_event(xrdpModule* mod, DWORD flags, DWORD code, DWORD keyboardType)
{
	int length;
	int status;
	wStream* s;
	XRDP_MSG_SCANCODE_KEYBOARD_EVENT msg;

	msg.msgFlags = 0;
	msg.type = XRDP_CLIENT_SCANCODE_KEYBOARD_EVENT;

	msg.flags = flags;
	msg.code = code;
	msg.keyboardType = keyboardType;

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_scancode_keyboard_event(NULL, &msg);
	xrdp_write_scancode_keyboard_event(s, &msg);

	status = freerds_named_pipe_write(mod->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_virtual_keyboard_event(xrdpModule* mod, DWORD flags, DWORD code)
{
	int length;
	int status;
	wStream* s;
	XRDP_MSG_VIRTUAL_KEYBOARD_EVENT msg;

	msg.msgFlags = 0;
	msg.type = XRDP_CLIENT_VIRTUAL_KEYBOARD_EVENT;

	msg.flags = flags;
	msg.code = code;

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_virtual_keyboard_event(NULL, &msg);
	xrdp_write_virtual_keyboard_event(s, &msg);

	status = freerds_named_pipe_write(mod->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_unicode_keyboard_event(xrdpModule* mod, DWORD flags, DWORD code)
{
	int length;
	int status;
	wStream* s;
	XRDP_MSG_UNICODE_KEYBOARD_EVENT msg;

	msg.msgFlags = 0;
	msg.type = XRDP_CLIENT_UNICODE_KEYBOARD_EVENT;

	msg.flags = flags;
	msg.code = code;

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_unicode_keyboard_event(NULL, &msg);
	xrdp_write_unicode_keyboard_event(s, &msg);

	status = freerds_named_pipe_write(mod->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_mouse_event(xrdpModule* mod, DWORD flags, DWORD x, DWORD y)
{
	int length;
	int status;
	wStream* s;
	XRDP_MSG_MOUSE_EVENT msg;

	msg.msgFlags = 0;
	msg.type = XRDP_CLIENT_MOUSE_EVENT;

	msg.flags = flags;
	msg.x = x;
	msg.y = y;

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_mouse_event(NULL, &msg);
	xrdp_write_mouse_event(s, &msg);

	status = freerds_named_pipe_write(mod->hClientPipe, Stream_Buffer(s), length);

	return status;
}

int freerds_client_outbound_extended_mouse_event(xrdpModule* mod, DWORD flags, DWORD x, DWORD y)
{
	int length;
	int status;
	wStream* s;
	XRDP_MSG_EXTENDED_MOUSE_EVENT msg;

	msg.msgFlags = 0;
	msg.type = XRDP_CLIENT_EXTENDED_MOUSE_EVENT;

	msg.flags = flags;
	msg.x = x;
	msg.y = y;

	s = mod->SendStream;
	Stream_SetPosition(s, 0);

	length = xrdp_write_extended_mouse_event(NULL, &msg);
	xrdp_write_extended_mouse_event(s, &msg);

	status = freerds_named_pipe_write(mod->hClientPipe, Stream_Buffer(s), length);

	return status;
}

xrdpClientModule* freerds_client_outbound_interface_new()
{
	xrdpClientModule* client;

	client = (xrdpClientModule*) malloc(sizeof(xrdpClientModule));

	if (client)
	{
		ZeroMemory(client, sizeof(xrdpClientModule));

		client->SynchronizeKeyboardEvent = freerds_client_outbound_synchronize_keyboard_event;
		client->ScancodeKeyboardEvent = freerds_client_outbound_scancode_keyboard_event;
		client->VirtualKeyboardEvent = freerds_client_outbound_virtual_keyboard_event;
		client->UnicodeKeyboardEvent = freerds_client_outbound_unicode_keyboard_event;
		client->MouseEvent = freerds_client_outbound_mouse_event;
		client->ExtendedMouseEvent = freerds_client_outbound_extended_mouse_event;
	}

	return client;
}

xrdpServerModule* freerds_server_outbound_interface_new()
{
	xrdpServerModule* server;

	server = (xrdpServerModule*) malloc(sizeof(xrdpServerModule));

	if (server)
	{
		ZeroMemory(server, sizeof(xrdpServerModule));
	}

	return server;
}
