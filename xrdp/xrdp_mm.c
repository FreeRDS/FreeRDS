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
 * module manager
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xrdp.h"
#include "log.h"

xrdpMm* xrdp_mm_create(xrdpSession* session)
{
	xrdpMm* self;

	self = (xrdpMm*) g_malloc(sizeof(xrdpMm), 1);
	self->session = session;

	xrdp_mm_connect(self);

	return self;
}

void xrdp_mm_delete(xrdpMm* self)
{
	if (!self)
		return;

	self->mod = NULL;
	trans_delete(self->sesman_trans);
	self->sesman_trans = 0;
	self->sesman_trans_up = 0;
	free(self);
}

int xrdp_auth_write_string(wStream* s, char* str)
{
	int length = 0;

	if (str)
		length = strlen(str);

	if (length < 0)
		length = 0;

	Stream_Write_UINT16(s, length);

	if (length)
		Stream_Write(s, str, length);

	return 0;
}

int xrdp_mm_send_login(xrdpMm* self)
{
	int status;
	int length;
	wStream* s;
	int ColorDepth;
	rdpSettings* settings;

	settings = self->session->settings;

	if (!settings->Username || !settings->Password)
		return 1;

	s = trans_get_out_s(self->sesman_trans, 8192);
	Stream_Seek(s, 8);

	ColorDepth = 24;

	Stream_Write_UINT16(s, 0);

	xrdp_auth_write_string(s, settings->Username);
	xrdp_auth_write_string(s, settings->Password);

	Stream_Write_UINT16(s, settings->DesktopWidth);
	Stream_Write_UINT16(s, settings->DesktopHeight);
	Stream_Write_UINT16(s, ColorDepth);

	xrdp_auth_write_string(s, settings->Domain);
	xrdp_auth_write_string(s, settings->AlternateShell);
	xrdp_auth_write_string(s, settings->ShellWorkingDirectory);
	xrdp_auth_write_string(s, settings->ClientAddress);

	length = (int) (Stream_Pointer(s) - Stream_Buffer(s));
	Stream_SetPosition(s, 0);

	Stream_Write_UINT32(s, 0); /* version */
	Stream_Write_UINT32(s, length); /* size */

	Stream_SetPosition(s, length);

	status = trans_force_write(self->sesman_trans);

	return status;
}

int xrdp_mm_process_login_response(xrdpMm* self, wStream* s)
{
	int ok;
	int display;
	int status;

	status = 0;
	Stream_Read_UINT16(s, ok);
	Stream_Read_UINT16(s, display);

	if (ok)
	{
		self->display = display;
		xrdp_mm_setup_mod(self);
	}
	else
	{
		log_message(LOG_LEVEL_INFO, "xrdp_mm_process_login_response: "
			"login failed");
	}

	return status;
}

int xrdp_mm_setup_mod(xrdpMm* self)
{
	int status;
	xrdpModule* mod;
	rdpSettings* settings;

	if (!self)
		return 1;

	mod = (xrdpModule*) malloc(sizeof(xrdpModule));
	self->mod = mod;

	if (!mod)
		return 1;

	ZeroMemory(mod, sizeof(xrdpModule));

	mod->size = sizeof(xrdpModule);
	mod->version = 2;
	mod->handle = (long) mod;
	self->mod->session = self->session;
	mod->settings = self->session->settings;

	xrdp_client_module_init(mod);
	xrdp_server_module_init(mod);

	status = 1;
	settings = self->session->settings;

	if (WaitForSingleObject(xrdp_process_get_term_event(self->session), 0) != WAIT_OBJECT_0)
	{
		if (self->mod->client->Start(self->mod, settings->DesktopWidth,
				settings->DesktopHeight, settings->ColorDepth) != 0)
		{
			SetEvent(xrdp_process_get_term_event(self->session)); /* kill session */
		}
	}

	if (WaitForSingleObject(xrdp_process_get_term_event(self->session), 0) != WAIT_OBJECT_0)
	{
		if (self->display > 0)
		{
			self->mod->settings = self->session->settings;
			self->mod->SessionId = self->display;
		}
	}

	if (WaitForSingleObject(xrdp_process_get_term_event(self->session), 0) != WAIT_OBJECT_0)
	{
		if (self->mod->client->Connect(self->mod) == 0)
		{
			status = 0; /* connect success */
		}
	}

	return status;
}

/* This is the callback registered for sesman communication replies. */
static int xrdp_mm_sesman_data_in(struct trans *trans)
{
	xrdpMm* self;
	wStream* s;
	int version;
	int size;
	int error;
	int code;

	if (!trans)
		return 1;

	self = (xrdpMm*) (trans->callback_data);
	s = trans_get_in_s(trans);

	if (!s)
		return 1;

	Stream_Read_UINT32(s, version);
	Stream_Read_UINT32(s, size);

	error = trans_force_read(trans, size - 8);

	if (error == 0)
	{
		Stream_Read_UINT16(s, code);

		switch (code)
		{
			/* even when the request is denied the reply will hold 3 as the command. */
			case 3:
				error = xrdp_mm_process_login_response(self, s);
				break;

			default:
				log_message(LOG_LEVEL_ERROR, "Fatal xrdp_mm_sesman_data_in: unknown cmd code %d", code);
				self->delete_sesman_trans = 1;
				self->connected_state = 0;
				break;
		}
	}

	return error;
}

int xrdp_mm_connect(xrdpMm* self)
{
	int ok;
	int status;
	char ip[256];
	char port[8];
	char errstr[256];

	self->connected_state = 0; /* true if connected to sesman else false */
	self->sesman_trans = NULL; /* connection to sesman */
	self->sesman_trans_up = 0; /* true once connected to sesman */
	self->delete_sesman_trans = 0; /* boolean set when done with sesman connection */
	self->display = 0; /* 10 for :10.0, 11 for :11.0, etc */

	g_memset(ip, 0, sizeof(ip));
	g_memset(errstr, 0, sizeof(errstr));
	g_memset(port, 0, sizeof(port));
	status = 0; /* success */

	g_strncpy(ip, "127.0.0.1", 255);

	ok = 0;
	trans_delete(self->sesman_trans);
	self->sesman_trans = trans_create(TRANS_MODE_TCP, 8192, 8192);
	strcpy(port, "3350");

	/* xrdp_mm_sesman_data_in is the callback that is called when data arrives */
	self->sesman_trans->trans_data_in = xrdp_mm_sesman_data_in;
	self->sesman_trans->header_size = 8;
	self->sesman_trans->callback_data = self;

	if (trans_connect(self->sesman_trans, ip, port, 3000) == 0)
	{
		self->sesman_trans_up = 1;
		ok = 1;
	}

	if (ok)
	{
		/* fully connect */
		self->connected_state = 1;
		status = xrdp_mm_send_login(self);
	}
	else
	{
		log_message(LOG_LEVEL_ERROR, errstr);
		trans_delete(self->sesman_trans);
		self->sesman_trans = 0;
		self->sesman_trans_up = 0;
		status = 1;
	}

	log_message(LOG_LEVEL_DEBUG, "returnvalue from xrdp_mm_connect %d", status);

	return status;
}

int xrdp_mm_get_event_handles(xrdpMm* self, HANDLE* events, DWORD* nCount)
{
	if (!self)
		return 0;

	if (self->sesman_trans && self->sesman_trans_up)
	{
		trans_get_event_handles(self->sesman_trans, events, nCount);
	}

	if (self->mod)
	{
		if (self->mod->client->GetEventHandles)
			self->mod->client->GetEventHandles(self->mod, events, nCount);
	}

	return 0;
}

int xrdp_mm_check_wait_objs(xrdpMm* self)
{
	int status;

	if (!self)
		return 0;

	status = 0;

	if ((self->sesman_trans != 0) && self->sesman_trans_up)
	{
		if (trans_check_wait_objs(self->sesman_trans) != 0)
		{
			self->delete_sesman_trans = 1;
		}
	}

	if (self->mod != 0)
	{
		if (self->mod->client->CheckEventHandles != 0)
		{
			status = self->mod->client->CheckEventHandles(self->mod);
		}
	}

	if (self->delete_sesman_trans)
	{
		trans_delete(self->sesman_trans);
		self->sesman_trans = 0;
		self->sesman_trans_up = 0;
		self->delete_sesman_trans = 0;
	}

	return status;
}
