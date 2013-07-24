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

#define ACCESS
#include "xrdp.h"
#include "log.h"
#ifdef ACCESS
#ifndef USE_NOPAM
#include "security/_pam_types.h"
#endif
#endif

#include "xup.h"
#include "xrdp-freerdp.h"

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
	if (self == 0)
	{
		return;
	}

	/* free any module stuff */
	xrdp_mm_module_cleanup(self);
	trans_delete(self->sesman_trans);
	self->sesman_trans = 0;
	self->sesman_trans_up = 0;
	list_delete(self->login_names);
	list_delete(self->login_values);
	free(self);
}

/* Send login information to sesman */
static int xrdp_mm_send_login(xrdpMm* self)
{
	wStream* s;
	int rv;
	int index;
	int count;
	int xserverbpp;
	char *username;
	char *password;
	char *name;
	char *value;

	xrdp_wm_log_msg(self->wm, "sending login info to session manager, "
		"please wait...");

	username = 0;
	password = 0;
	self->code = 0;
	xserverbpp = 0;
	count = self->login_names->count;

	for (index = 0; index < count; index++)
	{
		name = (char *) list_get_item(self->login_names, index);
		value = (char *) list_get_item(self->login_values, index);

		if (g_strcasecmp(name, "username") == 0)
		{
			username = value;
		}
		else
		{
			if (g_strcasecmp(name, "password") == 0)
			{
				password = value;
			}
			else
			{
				if (g_strcasecmp(name, "code") == 0)
				{
					/* this code is either 0 for Xvnc or 10 for X11rdp */
					self->code = g_atoi(value);
				}
				else
				{
					if (g_strcasecmp(name, "xserverbpp") == 0)
					{
						xserverbpp = g_atoi(value);
					}
				}
			}
		}
	}

	if ((username == 0) || (password == 0))
	{
		xrdp_wm_log_msg(self->wm, "Error finding username and password");
		return 1;
	}

	s = trans_get_out_s(self->sesman_trans, 8192);
	s->pointer += 8;

	/* this code is either 0 for Xvnc or 10 for X11rdp */
	Stream_Write_UINT16_BE(s, self->code);
	index = g_strlen(username);
	Stream_Write_UINT16_BE(s, index);
	Stream_Write(s, username, index);
	index = g_strlen(password);

	Stream_Write_UINT16_BE(s, index);
	Stream_Write(s, password, index);
	Stream_Write_UINT16_BE(s, self->wm->screen->width);
	Stream_Write_UINT16_BE(s, self->wm->screen->height);

	if (xserverbpp > 0)
	{
		Stream_Write_UINT16_BE(s, xserverbpp);
	}
	else
	{
		Stream_Write_UINT16_BE(s, self->wm->screen->bpp);
	}

	/* send domain */
	if (self->wm->session->settings->Domain)
	{
		index = g_strlen(self->wm->session->settings->Domain);
		Stream_Write_UINT16_BE(s, index);
		Stream_Write(s, self->wm->session->settings->Domain, index);
	}
	else
	{
		Stream_Write_UINT16_BE(s, 0);
		/* Stream_Write(s, "", 0); */
	}

	/* send program / shell */
	index = g_strlen(self->wm->session->settings->AlternateShell);
	Stream_Write_UINT16_BE(s, index);
	Stream_Write(s, self->wm->session->settings->AlternateShell, index);

	/* send directory */
	index = g_strlen(self->wm->session->settings->ShellWorkingDirectory);
	Stream_Write_UINT16_BE(s, index);
	Stream_Write(s, self->wm->session->settings->ShellWorkingDirectory, index);

	/* send client ip */
	index = g_strlen(self->wm->session->settings->ClientAddress);
	Stream_Write_UINT16_BE(s, index);
	Stream_Write(s, self->wm->session->settings->ClientAddress, index);

	index = (int) (s->pointer - s->buffer);
	s->pointer = s->buffer;

	/* Version 0 of the protocol to sesman is currently used by XRDP */
	Stream_Write_UINT32_BE(s, 0); /* version */
	Stream_Write_UINT32_BE(s, index); /* size */

	s->pointer = s->buffer + index;

	rv = trans_force_write(self->sesman_trans);

	if (rv != 0)
	{
		xrdp_wm_log_msg(self->wm, "xrdp_mm_send_login: xrdp_mm_send_login failed");
	}

	return rv;
}

/* returns error */
/* this goes through the login_names looking for one called 'aname'
 then it copies the corresponding login_values item into 'dest'
 'dest' must be at least 'dest_len' + 1 bytes in size */
static int xrdp_mm_get_value(xrdpMm* self, char *aname, char *dest, int dest_len)
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
		name = (char *) list_get_item(self->login_names, index);
		value = (char *) list_get_item(self->login_values, index);

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

static int xrdp_mm_setup_mod1(xrdpMm* self)
{
	char lib[256];
	char text[256];
	int client_module = 0;

	log_message(LOG_LEVEL_INFO, "xrdp_mm_setup_mod1");

	if (self == 0)
	{
		return 1;
	}

	lib[0] = 0;

	if (xrdp_mm_get_value(self, "lib", lib, 255) != 0)
	{
		g_snprintf(text, 255, "no library name specified in xrdp.ini, please add "
			"lib=libxrdp-vnc.so or similar");
		xrdp_wm_log_msg(self->wm, text);

		return 1;
	}

	if (lib[0] == 0)
	{
		g_snprintf(text, 255, "empty library name specified in xrdp.ini, please "
			"add lib=libxrdp-vnc.so or similar");
		xrdp_wm_log_msg(self->wm, text);

		return 1;
	}

	if (self->mod_handle == 0)
	{
		if (strcmp(lib, "libxrdp-ng-freerdp.so") == 0)
			client_module = 1;

		if (!client_module)
		{
			self->ModuleInit = xup_module_init;
			self->ModuleExit = xup_module_exit;
			self->mod_handle = 1;
		}
		else
		{
			self->ModuleInit = freerdp_client_module_init;
			self->ModuleExit = freerdp_client_module_exit;
			self->mod_handle = 1;
		}

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

				mod->server = (xrdpServerModule*) malloc(sizeof(xrdpServerModule));

				if (mod->server)
				{
					ZeroMemory(mod->server, sizeof(xrdpServerModule));

					mod->server->BeginUpdate = server_begin_update;
					mod->server->EndUpdate = server_end_update;
					mod->server->Beep = server_bell_trigger;
					mod->server->OpaqueRect = server_opaque_rect;
					mod->server->ScreenBlt = server_screen_blt;
					mod->server->PaintRect = server_paint_rect;
					mod->server->SetPointer = server_set_pointer;
					mod->server->SetPalette = server_palette;
					mod->server->SetClippingRegion = server_set_clip;
					mod->server->SetNullClippingRegion = server_reset_clip;
					mod->server->SetForeColor = server_set_fgcolor;
					mod->server->SetBackColor = server_set_bgcolor;
					mod->server->SetRop2 = server_set_opcode;
					mod->server->SetMixMode = server_set_mixmode;
					mod->server->SetBrush = server_set_brush;
					mod->server->SetPen = server_set_pen;
					mod->server->LineTo = server_draw_line;
					mod->server->AddChar = server_add_char;
					mod->server->Text = server_draw_text;
					mod->server->Reset = server_reset;
					mod->server->CreateOffscreenSurface = server_create_os_surface;
					mod->server->SwitchOffscreenSurface = server_switch_os_surface;
					mod->server->DeleteOffscreenSurface = server_delete_os_surface;
					mod->server->PaintOffscreenRect = server_paint_rect_os;
					mod->server->WindowNewUpdate = server_window_new_update;
					mod->server->WindowDelete = server_window_delete;
					mod->server->WindowIcon = server_window_icon;
					mod->server->WindowCachedIcon = server_window_cached_icon;
					mod->server->NotifyNewUpdate = server_notify_new_update;
					mod->server->NotifyDelete = server_notify_delete;
					mod->server->MonitoredDesktop = server_monitored_desktop;
				}

				self->ModuleInit(mod);
			}

			if (self->mod != 0)
			{
				g_writeln("loaded module '%s' ok, interface size %d, version %d", lib,
						self->mod->size, self->mod->version);
			}
		}
		else
		{
			log_message(LOG_LEVEL_ERROR, "no ModuleInit or ModuleExit address found");
		}
	}

	/* id self->mod is null, there must be a problem */
	if (self->mod == 0)
	{
		DEBUG(("problem loading lib in xrdp_mm_setup_mod1"));
		return 1;
	}

	self->mod->wm = (long) (self->wm);

	return 0;
}

static int xrdp_mm_setup_mod2(xrdpMm* self)
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
			if (self->code == 0) /* Xvnc */
			{
				g_snprintf(text, 255, "%d", 5900 + self->display);
			}
			else
			{
				if (self->code == 10) /* X11rdp */
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
				else
				{
					SetEvent(xrdp_process_get_term_event(self->wm->pro_layer)); /* kill session */
				}
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

static void cleanup_sesman_connection(xrdpMm* self)
{
	self->delete_sesman_trans = 1;
	self->connected_state = 0;

	if (self->wm->login_mode != 10)
	{
		xrdp_wm_set_login_mode(self->wm, 11);
		xrdp_mm_module_cleanup(self);
	}
}

static int xrdp_mm_process_login_response(xrdpMm* self, wStream* s)
{
	int ok;
	int display;
	int rv;
	char text[256];
	char ip[256];

	rv = 0;
	Stream_Read_UINT16_BE(s, ok);
	Stream_Read_UINT16_BE(s, display);

	if (ok)
	{
		self->display = display;
		g_snprintf(text, 255, "xrdp_mm_process_login_response: login successful "
			"for display %d", display);
		xrdp_wm_log_msg(self->wm, text);

		if (xrdp_mm_setup_mod1(self) == 0)
		{
			if (xrdp_mm_setup_mod2(self) == 0)
			{
				xrdp_mm_get_value(self, "ip", ip, 255);
				xrdp_wm_set_login_mode(self->wm, 10);
				self->wm->dragging = 0;
			}
		}
	}
	else
	{
		xrdp_wm_log_msg(self->wm, "xrdp_mm_process_login_response: "
			"login failed");
		log_message(LOG_LEVEL_INFO, "xrdp_mm_process_login_response: "
			"login failed");
	}

	cleanup_sesman_connection(self);
	return rv;
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
				val = (char *) list_get_item(names, index);

				if (val != 0)
				{
					if (g_strcasecmp(val, "ListenPort") == 0)
					{
						val = (char *) list_get_item(values, index);
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

	if (trans == 0)
	{
		return 1;
	}

	self = (xrdpMm *) (trans->callback_data);
	s = trans_get_in_s(trans);

	if (s == 0)
	{
		return 1;
	}

	Stream_Read_UINT32_BE(s, version);
	Stream_Read_UINT32_BE(s, size);
	error = trans_force_read(trans, size - 8);

	if (error == 0)
	{
		Stream_Read_UINT16_BE(s, code);

		switch (code)
		{
			/* even when the request is denied the reply will hold 3 as the command. */
			case 3:
				error = xrdp_mm_process_login_response(self, s);
				break;
			default:
				xrdp_wm_log_msg(self->wm, "An undefined reply code was received from sesman");
				log_message(LOG_LEVEL_ERROR, "Fatal xrdp_mm_sesman_data_in: unknown cmd code %d", code);
				cleanup_sesman_connection(self);
				break;
		}
	}

	return error;
}

#ifdef ACCESS
#ifndef USE_NOPAM
/* return 0 on success */
static int access_control(char *username, char *password, char *srv)
{
	int reply;
	int rec = 32 + 1; /* 32 is reserved for PAM failures this means connect failure */
	wStream* in_s;
	wStream* out_s;
	unsigned long version;
	unsigned short int dummy;
	unsigned short int pAM_errorcode;
	unsigned short int code;
	unsigned long size;
	int index;
	int socket = g_tcp_socket();

	if (socket > 0)
	{
		/* we use a blocking socket here */
		reply = g_tcp_connect(socket, srv, "3350");

		if (reply == 0)
		{
			in_s = Stream_New(NULL, 500);
			in_s->length = 0;

			out_s = Stream_New(NULL, 500);
			Stream_Seek(out_s, 8);

			Stream_Write_UINT16_BE(out_s, 4); /*0x04 means SCP_GW_AUTHENTICATION*/
			index = g_strlen(username);
			Stream_Write_UINT16_BE(out_s, index);
			Stream_Write(out_s, username, index);

			index = g_strlen(password);
			Stream_Write_UINT16_BE(out_s, index);
			Stream_Write(out_s, password, index);

			index = (int) (out_s->pointer - out_s->buffer);
			out_s->pointer = out_s->buffer;

			Stream_Write_UINT32_BE(out_s, 0); /* version */
			Stream_Write_UINT32_BE(out_s, index); /* size */

			out_s->pointer = out_s->buffer + index;

			reply = g_tcp_send(socket, Stream_Buffer(out_s), index, 0);
			Stream_Free(out_s, TRUE);

			if (reply > 0)
			{
				/* We wait in 5 sec for a reply from sesman*/
				if (g_tcp_can_recv(socket, 5000))
				{
					reply = g_tcp_recv(socket, in_s->buffer + in_s->length, 500, 0);

					if (reply > 0)
					{
						in_s->length += reply;
						Stream_Read_UINT32_BE(in_s, version);
						/*g_writeln("Version number in reply from sesman: %d",version) ; */
						Stream_Read_UINT32_BE(in_s, size);

						if ((size == 14) && (version == 0))
						{
							Stream_Read_UINT16_BE(in_s, code);
							Stream_Read_UINT16_BE(in_s, pAM_errorcode); /* this variable holds the PAM error code if the variable is >32 it is a "invented" code */
							Stream_Read_UINT16_BE(in_s, dummy);

							if (code != 4) /*0x04 means SCP_GW_AUTHENTICATION*/
							{
								log_message(LOG_LEVEL_ERROR, "Returned cmd code from "
									"sesman is corrupt");
							}
							else
							{
								rec = pAM_errorcode; /* here we read the reply from the access control */
							}
						}
						else
						{
							log_message(LOG_LEVEL_ERROR, "Corrupt reply size or "
								"version from sesman: %d", size);
						}
					}
					else
					{
						log_message(LOG_LEVEL_ERROR, "No data received from sesman");
					}
				}
				else
				{
					log_message(LOG_LEVEL_ERROR, "Timeout when waiting for sesman");
				}
			}
			else
			{
				log_message(LOG_LEVEL_ERROR, "No success sending to sesman");
			}

			Stream_Free(in_s, TRUE);
			g_tcp_close(socket);
		}
		else
		{
			log_message(LOG_LEVEL_ERROR, "Failure connecting to socket sesman");
		}
	}
	else
	{
		log_message(LOG_LEVEL_ERROR, "Failure creating socket - for access control");
	}

	return rec;
}
#endif
#endif

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
		self->code = 0; /* 0 Xvnc session 10 X11rdp session */
		self->sesman_controlled = 0; /* true if this is a sesman session */
	}
}

#ifdef ACCESS
#ifndef USE_NOPAM
static const char* getPAMError(const int pamError, char *text, int text_bytes)
{
	switch (pamError)
	{
		case PAM_SUCCESS:
			return "Success";
		case PAM_OPEN_ERR:
			return "dlopen() failure";
		case PAM_SYMBOL_ERR:
			return "Symbol not found";
		case PAM_SERVICE_ERR:
			return "Error in service module";
		case PAM_SYSTEM_ERR:
			return "System error";
		case PAM_BUF_ERR:
			return "Memory buffer error";
		case PAM_PERM_DENIED:
			return "Permission denied";
		case PAM_AUTH_ERR:
			return "Authentication failure";
		case PAM_CRED_INSUFFICIENT:
			return "Insufficient credentials to access authentication data";
		case PAM_AUTHINFO_UNAVAIL:
			return "Authentication service cannot retrieve authentication info.";
		case PAM_USER_UNKNOWN:
			return "User not known to the underlying authentication module";
		case PAM_MAXTRIES:
			return "Have exhasted maximum number of retries for service.";
		case PAM_NEW_AUTHTOK_REQD:
			return "Authentication token is no longer valid; new one required.";
		case PAM_ACCT_EXPIRED:
			return "User account has expired";
		case PAM_CRED_UNAVAIL:
			return "Authentication service cannot retrieve user credentials";
		case PAM_CRED_EXPIRED:
			return "User credentials expired";
		case PAM_CRED_ERR:
			return "Failure setting user credentials";
		case PAM_NO_MODULE_DATA:
			return "No module specific data is present";
		case PAM_BAD_ITEM:
			return "Bad item passed to pam_*_item()";
		case PAM_CONV_ERR:
			return "Conversation error";
		case PAM_AUTHTOK_ERR:
			return "Authentication token manipulation error";
		case PAM_AUTHTOK_LOCK_BUSY:
			return "Authentication token lock busy";
		case PAM_AUTHTOK_DISABLE_AGING:
			return "Authentication token aging disabled";
		case PAM_TRY_AGAIN:
			return "Failed preliminary check by password service";
		case PAM_IGNORE:
			return "Please ignore underlying account module";
		case PAM_MODULE_UNKNOWN:
			return "Module is unknown";
		case PAM_AUTHTOK_EXPIRED:
			return "Authentication token expired";
		case PAM_CONV_AGAIN:
			return "Conversation is waiting for event";
		case PAM_INCOMPLETE:
			return "Application needs to call libpam again";
		case 32 + 1:
			return "Error connecting to PAM";
		case 32 + 3:
			return "Username okay but group problem";
		default:
			g_snprintf(text, text_bytes, "Not defined PAM error:%d", pamError);
			return text;
	}
}

static const char* getPAMAdditionalErrorInfo(const int pamError, xrdpMm* self)
{
	switch (pamError)
	{
		case PAM_SUCCESS:
			return NULL;
		case PAM_OPEN_ERR:
		case PAM_SYMBOL_ERR:
		case PAM_SERVICE_ERR:
		case PAM_SYSTEM_ERR:
		case PAM_BUF_ERR:
		case PAM_PERM_DENIED:
		case PAM_AUTH_ERR:
		case PAM_CRED_INSUFFICIENT:
		case PAM_AUTHINFO_UNAVAIL:
		case PAM_USER_UNKNOWN:
		case PAM_CRED_UNAVAIL:
		case PAM_CRED_ERR:
		case PAM_NO_MODULE_DATA:
		case PAM_BAD_ITEM:
		case PAM_CONV_ERR:
		case PAM_AUTHTOK_ERR:
		case PAM_AUTHTOK_LOCK_BUSY:
		case PAM_AUTHTOK_DISABLE_AGING:
		case PAM_TRY_AGAIN:
		case PAM_IGNORE:
		case PAM_MODULE_UNKNOWN:
		case PAM_CONV_AGAIN:
		case PAM_INCOMPLETE:
		case _PAM_RETURN_VALUES + 1:
		case _PAM_RETURN_VALUES + 3:
			return NULL;
		case PAM_MAXTRIES:
		case PAM_NEW_AUTHTOK_REQD:
		case PAM_ACCT_EXPIRED:
		case PAM_CRED_EXPIRED:
		case PAM_AUTHTOK_EXPIRED:
			if (self->wm->pamerrortxt[0])
			{
				return self->wm->pamerrortxt;
			} else
			{
				return "Authentication error - Verify that user/password is valid";
			}
		default:
			return "No expected error";
	}
}
#endif
#endif

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
#ifdef ACCESS
#ifndef USE_NOPAM
	int use_pam_auth = 0;
	char pam_auth_sessionIP[256];
	char pam_auth_password[256];
	char pam_auth_username[256];
#endif
	char username[256];
	char password[256];
	username[0] = 0;
	password[0] = 0;
#endif
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
		name = (char *) list_get_item(names, index);
		value = (char *) list_get_item(values, index);

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

#ifdef ACCESS
#ifndef USE_NOPAM
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
#endif
	}

#ifdef ACCESS
#ifndef USE_NOPAM
	if (use_pam_auth)
	{
		int reply;
		char replytxt[128];
		char pam_error[128];
		const char *additionalError;
		xrdp_wm_log_msg(self->wm, "Please wait, we now perform access control...");

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
		reply = access_control(pam_auth_username, pam_auth_password, pam_auth_sessionIP);

		g_sprintf(replytxt, "Reply from access control: %s", getPAMError(reply, pam_error, 127));

		xrdp_wm_log_msg(self->wm, replytxt);
		log_message(LOG_LEVEL_INFO, replytxt);
		additionalError = getPAMAdditionalErrorInfo(reply, self);

		if (additionalError)
		{
			g_snprintf(replytxt, 127, "%s", additionalError);
			if (replytxt[0])
			{
				xrdp_wm_log_msg(self->wm, replytxt);
			}
		}

		if (reply != 0)
		{
			rv = 1;
			return rv;
		}
	}
#endif
#endif

	if (self->sesman_controlled)
	{
		ok = 0;
		trans_delete(self->sesman_trans);
		self->sesman_trans = trans_create(TRANS_MODE_TCP, 8192, 8192);
		xrdp_mm_get_sesman_port(port, sizeof(port));
		g_snprintf(text, 255, "connecting to sesman ip %s port %s", ip, port);
		xrdp_wm_log_msg(self->wm, text);
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
			xrdp_wm_log_msg(self->wm, "sesman connect ok");
			self->connected_state = 1;
			rv = xrdp_mm_send_login(self);
		}
		else
		{
			g_snprintf(errstr, 255, "Failure to connect to sesman: %s port: %s", ip, port);
			xrdp_wm_log_msg(self->wm, errstr);
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
				xrdp_wm_set_login_mode(self->wm, 10);
				rv = 0; /*sucess*/
			}
			else
			{
				/* connect error */
				g_snprintf(errstr, 255, "Failure to connect to: %s", ip);
				log_message(LOG_LEVEL_ERROR, errstr);
				xrdp_wm_log_msg(self->wm, errstr);
				rv = 1; /* failure */
			}
		}
		else
		{
			log_message(LOG_LEVEL_ERROR, "Failure setting up module");
		}

		if (self->wm->login_mode != 10)
		{
			xrdp_wm_set_login_mode(self->wm, 11);
			xrdp_mm_module_cleanup(self);
			rv = 1; /* failure */
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
