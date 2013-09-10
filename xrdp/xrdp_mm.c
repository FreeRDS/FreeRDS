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

#include "xrdp_xup.h"

xrdpMm* xrdp_mm_create(xrdpSession* session)
{
	xrdpMm* self;

	self = (xrdpMm*) g_malloc(sizeof(xrdpMm), 1);
	self->session = session;

	xrdp_mm_connect(self);

	return self;
}

static void xrdp_mm_module_cleanup(xrdpMm* self)
{
	log_message(LOG_LEVEL_DEBUG, "xrdp_mm_module_cleanup");

	if (self->mod != 0)
	{
		if (self->ModuleExit)
		{
			/* let the module cleanup */
			self->ModuleExit(self->mod);
		}
	}

	self->ModuleInit = 0;
	self->ModuleExit = 0;
	self->mod = 0;
	self->mod_handle = 0;
}

void xrdp_mm_delete(xrdpMm* self)
{
	if (!self)
		return;

	/* free any module stuff */
	xrdp_mm_module_cleanup(self);
	trans_delete(self->sesman_trans);
	self->sesman_trans = 0;
	self->sesman_trans_up = 0;
	free(self);
}

int xrdp_mm_setup_mod1(xrdpMm* self)
{
	log_message(LOG_LEVEL_INFO, "xrdp_mm_setup_mod1");

	if (!self)
		return 1;

	if (self->mod_handle == 0)
	{
		self->ModuleInit = xup_module_init;
		self->ModuleExit = xup_module_exit;
		self->mod_handle = 1;

		if ((self->ModuleInit != 0) && (self->ModuleExit != 0))
		{
			xrdpModule* mod;

			mod = (xrdpModule*) malloc(sizeof(xrdpModule));
			self->mod = mod;

			if (self->mod)
			{
				ZeroMemory(mod, sizeof(xrdpModule));

				mod->size = sizeof(xrdpModule);
				mod->version = 2;
				mod->handle = (long) mod;

				xrdp_server_module_init(mod);

				mod->settings = self->session->settings;

				self->ModuleInit(mod);
			}

			if (self->mod)
			{
				g_writeln("loaded module ok, interface size %d, version %d",
						self->mod->size, self->mod->version);
			}
		}
		else
		{
			log_message(LOG_LEVEL_ERROR, "no ModuleInit or ModuleExit address found");
		}
	}

	if (!self->mod)
	{
		DEBUG(("problem loading lib in xrdp_mm_setup_mod1"));
		return 1;
	}

	self->mod->session = self->session;

	return 0;
}

int xrdp_mm_setup_mod2(xrdpMm* self)
{
	int status;
	rdpSettings* settings;

	log_message(LOG_LEVEL_INFO, "xrdp_mm_setup_mod2");

	status = 1; /* failure */

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
			g_snprintf(self->mod->port, 255, "/tmp/.xrdp/xrdp_display_%d", self->display);
			self->mod->settings = self->session->settings;
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

void xrdp_mm_cleanup_sesman_connection(xrdpMm* self)
{
	self->delete_sesman_trans = 1;
	self->connected_state = 0;
	xrdp_mm_module_cleanup(self);
}

static int xrdp_mm_get_sesman_port(char *port, int port_bytes)
{
	int fd;
	int error;
	int index;
	char *val;
	char cfg_file[256];
	xrdpList* names;
	xrdpList* values;

	g_memset(cfg_file, 0, sizeof(char) * 256);
	/* default to port 3350 */
	g_strncpy(port, "3350", port_bytes - 1);
	/* see if port is in xrdp.ini file */
	g_snprintf(cfg_file, 255, "%s/sesman.ini", XRDP_CFG_PATH);
	fd = g_file_open(cfg_file);

	if (fd > 0)
	{
		names = list_create();
		names->auto_free = 1;
		values = list_create();
		values->auto_free = 1;

		if (file_read_section(fd, "Globals", names, values) == 0)
		{
			for (index = 0; index < names->count; index++)
			{
				val = (char*) list_get_item(names, index);

				if (val != 0)
				{
					if (g_strcasecmp(val, "ListenPort") == 0)
					{
						val = (char*) list_get_item(values, index);
						error = g_atoi(val);

						if ((error > 0) && (error < 65000))
						{
							g_strncpy(port, val, port_bytes - 1);
						}

						break;
					}
				}
			}
		}

		list_delete(names);
		list_delete(values);
		g_file_close(fd);
	}

	return 0;
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
				xrdp_mm_cleanup_sesman_connection(self);
				break;
		}
	}

	return error;
}

/* This routine clears all states to make sure that our next login will be
 * as expected. If the user does not press ok on the log window and try to
 * connect again we must make sure that no previous information is stored.*/
static void cleanup_states(xrdpMm* self)
{
	if (self != NULL)
	{
		self->connected_state = 0; /* true if connected to sesman else false */
		self->sesman_trans = NULL; /* connection to sesman */
		self->sesman_trans_up = 0; /* true once connected to sesman */
		self->delete_sesman_trans = 0; /* boolean set when done with sesman connection */
		self->display = 0; /* 10 for :10.0, 11 for :11.0, etc */
		self->sesman_controlled = 0; /* true if this is a sesman session */
	}
}

int xrdp_mm_connect(xrdpMm* self)
{
	int ok;
	int status;
	char ip[256];
	char port[8];
	char errstr[256];

	/* make sure we start in correct state */
	cleanup_states(self);
	g_memset(ip, 0, sizeof(ip));
	g_memset(errstr, 0, sizeof(errstr));
	g_memset(port, 0, sizeof(port));
	status = 0; /* success */

	g_strncpy(ip, "127.0.0.1", 255);

	self->sesman_controlled = 1;

	ok = 0;
	trans_delete(self->sesman_trans);
	self->sesman_trans = trans_create(TRANS_MODE_TCP, 8192, 8192);
	xrdp_mm_get_sesman_port(port, sizeof(port));

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
