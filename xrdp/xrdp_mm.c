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

#ifdef WITH_PAM
#include "security/_pam_types.h"
#endif

#include "xup.h"

xrdpMm* xrdp_mm_create(xrdpWm *owner)
{
	xrdpMm* self;

	self = (xrdpMm*) g_malloc(sizeof(xrdpMm), 1);
	self->wm = owner;
	self->login_names = list_create();
	self->login_names->auto_free = 1;
	self->login_values = list_create();
	self->login_values->auto_free = 1;

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
	list_delete(self->login_names);
	list_delete(self->login_values);
	free(self);
}

/* returns error */
/* this goes through the login_names looking for one called 'aname'
 then it copies the corresponding login_values item into 'dest'
 'dest' must be at least 'dest_len' + 1 bytes in size */
int xrdp_mm_get_value(xrdpMm* self, char *aname, char *dest, int dest_len)
{
	char *name;
	char *value;
	int index;
	int count;
	int rv;

	rv = 1;
	/* find the library name */
	dest[0] = 0;
	count = self->login_names->count;

	for (index = 0; index < count; index++)
	{
		name = (char*) list_get_item(self->login_names, index);
		value = (char*) list_get_item(self->login_values, index);

		if ((name == 0) || (value == 0))
		{
			break;
		}

		if (g_strcasecmp(name, aname) == 0)
		{
			g_strncpy(dest, value, dest_len);
			rv = 0;
		}
	}

	return rv;
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

	self->mod->wm = (long) (self->wm);
	self->mod->session = self->wm->session;

	return 0;
}

int xrdp_mm_setup_mod2(xrdpMm* self)
{
	char text[256];
	char *name;
	char *value;
	int i;
	int rv;
	int key_flags;
	int device_flags;
	int use_uds;

	log_message(LOG_LEVEL_INFO, "xrdp_mm_setup_mod2");

	rv = 1; /* failure */
	g_memset(text, 0, sizeof(text));

	if (WaitForSingleObject(xrdp_process_get_term_event(self->wm->pro_layer), 0) != WAIT_OBJECT_0)
	{
		if (self->mod->client->Start(self->mod, self->wm->screen->width,
				self->wm->screen->height, self->wm->screen->bpp) != 0)
		{
			SetEvent(xrdp_process_get_term_event(self->wm->pro_layer)); /* kill session */
		}
	}

	if (WaitForSingleObject(xrdp_process_get_term_event(self->wm->pro_layer), 0) != WAIT_OBJECT_0)
	{
		if (self->display > 0)
		{
			use_uds = 1;

			if (xrdp_mm_get_value(self, "ip", text, 255) == 0)
			{
				if (g_strcmp(text, "127.0.0.1") != 0)
				{
					use_uds = 0;
				}
			}

			if (use_uds)
			{
				g_snprintf(text, 255, "/tmp/.xrdp/xrdp_display_%d", self->display);
			}
			else
			{
				g_snprintf(text, 255, "%d", 6200 + self->display);
			}
		}
	}

	if (WaitForSingleObject(xrdp_process_get_term_event(self->wm->pro_layer), 0) != WAIT_OBJECT_0)
	{
		/* this adds the port to the end of the list, it will already be in
		 the list as -1 the module should use the last one */
		if (g_strlen(text) > 0)
		{
			list_add_item(self->login_names, (long) g_strdup("port"));
			list_add_item(self->login_values, (long) g_strdup(text));
		}

		/* always set these */
		self->mod->client->SetParam(self->mod, "settings", (char*) self->wm->session->settings);
		name = self->wm->session->settings->ServerHostname;
		self->mod->client->SetParam(self->mod, "hostname", name);
		g_snprintf(text, 255, "%d", self->wm->session->settings->KeyboardLayout);
		self->mod->client->SetParam(self->mod, "keylayout", text);

		for (i = 0; i < self->login_names->count; i++)
		{
			name = (char*) list_get_item(self->login_names, i);
			value = (char*) list_get_item(self->login_values, i);
			self->mod->client->SetParam(self->mod, name, value);
		}

		/* connect */
		if (self->mod->client->Connect(self->mod) == 0)
		{
			rv = 0; /* connect success */
		}
	}

	if (rv == 0)
	{
		/* sync modifiers */
		key_flags = 0;
		device_flags = 0;

		if (self->wm->scroll_lock)
		{
			key_flags |= 1;
		}

		if (self->wm->num_lock)
		{
			key_flags |= 2;
		}

		if (self->wm->caps_lock)
		{
			key_flags |= 4;
		}

		if (self->mod != 0)
		{
			if (self->mod->client->Event)
			{
				self->mod->client->Event(self->mod, 17, key_flags, device_flags, key_flags, device_flags);
			}
		}
	}

	return rv;
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
	xrdpList *names;
	xrdpList *values;
	int index;
	int count;
	int ok;
	int rv;
	char *name;
	char *value;
	char ip[256];
	char errstr[256];
	char text[256];
	char port[8];
#ifdef WITH_PAM
	int use_pam_auth = 0;
	char pam_auth_sessionIP[256];
	char pam_auth_password[256];
	char pam_auth_username[256];
#endif
	char username[256];
	char password[256];
	username[0] = 0;
	password[0] = 0;

	/* make sure we start in correct state */
	cleanup_states(self);
	g_memset(ip, 0, sizeof(ip));
	g_memset(errstr, 0, sizeof(errstr));
	g_memset(text, 0, sizeof(text));
	g_memset(port, 0, sizeof(port));
	rv = 0; /* success */
	names = self->login_names;
	values = self->login_values;
	count = names->count;

	for (index = 0; index < count; index++)
	{
		name = (char*) list_get_item(names, index);
		value = (char*) list_get_item(values, index);

		if (g_strcasecmp(name, "ip") == 0)
		{
			g_strncpy(ip, value, 255);
		}
		else if (g_strcasecmp(name, "port") == 0)
		{
			if (g_strcasecmp(value, "-1") == 0)
			{
				self->sesman_controlled = 1;
			}
		}

#ifdef WITH_PAM
		else if (g_strcasecmp(name, "pamusername") == 0)
		{
			use_pam_auth = 1;
			g_strncpy(pam_auth_username, value, 255);
		}
		else if (g_strcasecmp(name, "pamsessionmng") == 0)
		{
			g_strncpy(pam_auth_sessionIP, value, 255);
		}
		else if (g_strcasecmp(name, "pampassword") == 0)
		{
			g_strncpy(pam_auth_password, value, 255);
		}
#endif
		else if (g_strcasecmp(name, "password") == 0)
		{
			g_strncpy(password, value, 255);
		}
		else if (g_strcasecmp(name, "username") == 0)
		{
			g_strncpy(username, value, 255);
		}
	}

#ifdef WITH_PAM
	if (use_pam_auth)
	{
		int reply;
		char replytxt[128];
		char pam_error[128];
		const char *additionalError;

		/* g_writeln("we use pam modules to check if we can approve this user"); */
		if (!g_strncmp(pam_auth_username, "same", 255))
		{
			log_message(LOG_LEVEL_DEBUG, "pamusername copied from username - same: %s", username);
			g_strncpy(pam_auth_username, username, 255);
		}

		if (!g_strncmp(pam_auth_password, "same", 255))
		{
			log_message(LOG_LEVEL_DEBUG, "pam_auth_password copied from username - same: %s", password);
			g_strncpy(pam_auth_password, password, 255);
		}

		/* access_control return 0 on success */
		reply = xrdp_mm_access_control(pam_auth_username, pam_auth_password, pam_auth_sessionIP);

		g_sprintf(replytxt, "Reply from access control: %s", getPAMError(reply, pam_error, 127));

		log_message(LOG_LEVEL_INFO, replytxt);
		additionalError = getPAMAdditionalErrorInfo(reply, self);

		if (additionalError)
		{
			g_snprintf(replytxt, 127, "%s", additionalError);

			if (replytxt[0])
			{

			}
		}

		if (reply != 0)
		{
			rv = 1;
			return rv;
		}
	}
#endif

	if (self->sesman_controlled)
	{
		ok = 0;
		trans_delete(self->sesman_trans);
		self->sesman_trans = trans_create(TRANS_MODE_TCP, 8192, 8192);
		xrdp_mm_get_sesman_port(port, sizeof(port));

		/* xrdp_mm_sesman_data_in is the callback that is called when data arrives */
		self->sesman_trans->trans_data_in = xrdp_mm_sesman_data_in;
		self->sesman_trans->header_size = 8;
		self->sesman_trans->callback_data = self;

		/* try to connect up to 4 times */
		for (index = 0; index < 4; index++)
		{
			if (trans_connect(self->sesman_trans, ip, port, 3000) == 0)
			{
				self->sesman_trans_up = 1;
				ok = 1;
				break;
			}

			g_sleep(1000);
			g_writeln("xrdp_mm_connect: connect failed "
				"trying again...");
		}

		if (ok)
		{
			/* fully connect */
			self->connected_state = 1;
			rv = xrdp_mm_send_login(self);
		}
		else
		{
			log_message(LOG_LEVEL_ERROR, errstr);
			trans_delete(self->sesman_trans);
			self->sesman_trans = 0;
			self->sesman_trans_up = 0;
			rv = 1;
		}
	}
	else /* no sesman */
	{
		if (xrdp_mm_setup_mod1(self) == 0)
		{
			if (xrdp_mm_setup_mod2(self) == 0)
			{
				rv = 0;
			}
			else
			{
				/* connect error */
				g_snprintf(errstr, 255, "Failure to connect to: %s", ip);
				log_message(LOG_LEVEL_ERROR, errstr);
				rv = 1; /* failure */
			}
		}
		else
		{
			log_message(LOG_LEVEL_ERROR, "Failure setting up module");
		}
	}

	log_message(LOG_LEVEL_DEBUG, "returnvalue from xrdp_mm_connect %d", rv);

	return rv;
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
