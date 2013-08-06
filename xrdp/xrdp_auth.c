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

int xrdp_mm_send_login(xrdpMm* self)
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

	username = 0;
	password = 0;
	self->code = 0;
	xserverbpp = 0;
	count = self->login_names->count;

	for (index = 0; index < count; index++)
	{
		name = (char*) list_get_item(self->login_names, index);
		value = (char*) list_get_item(self->login_values, index);

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
		return 1;
	}

	s = trans_get_out_s(self->sesman_trans, 8192);
	Stream_Seek(s, 8);

	/* this code is either 0 for Xvnc or 10 for X11rdp */
	Stream_Write_UINT16(s, self->code);
	index = g_strlen(username);
	Stream_Write_UINT16(s, index);
	Stream_Write(s, username, index);
	index = g_strlen(password);

	Stream_Write_UINT16(s, index);
	Stream_Write(s, password, index);
	Stream_Write_UINT16(s, self->wm->screen->width);
	Stream_Write_UINT16(s, self->wm->screen->height);

	if (xserverbpp > 0)
	{
		Stream_Write_UINT16(s, xserverbpp);
	}
	else
	{
		Stream_Write_UINT16(s, self->wm->screen->bpp);
	}

	/* send domain */
	if (self->wm->session->settings->Domain)
	{
		index = g_strlen(self->wm->session->settings->Domain);
		Stream_Write_UINT16(s, index);
		Stream_Write(s, self->wm->session->settings->Domain, index);
	}
	else
	{
		Stream_Write_UINT16(s, 0);
		/* Stream_Write(s, "", 0); */
	}

	/* send program / shell */
	index = g_strlen(self->wm->session->settings->AlternateShell);
	Stream_Write_UINT16(s, index);
	Stream_Write(s, self->wm->session->settings->AlternateShell, index);

	/* send directory */
	index = g_strlen(self->wm->session->settings->ShellWorkingDirectory);
	Stream_Write_UINT16(s, index);
	Stream_Write(s, self->wm->session->settings->ShellWorkingDirectory, index);

	/* send client ip */
	index = g_strlen(self->wm->session->settings->ClientAddress);
	Stream_Write_UINT16(s, index);
	Stream_Write(s, self->wm->session->settings->ClientAddress, index);

	index = (int) (s->pointer - s->buffer);
	Stream_SetPosition(s, 0);

	/* Version 0 of the protocol to sesman is currently used by XRDP */
	Stream_Write_UINT32(s, 0); /* version */
	Stream_Write_UINT32(s, index); /* size */

	s->pointer = s->buffer + index;

	rv = trans_force_write(self->sesman_trans);

	if (rv != 0)
	{

	}

	return rv;
}

int xrdp_mm_process_login_response(xrdpMm* self, wStream* s)
{
	int ok;
	int display;
	int rv;
	char ip[256];

	rv = 0;
	Stream_Read_UINT16(s, ok);
	Stream_Read_UINT16(s, display);

	if (ok)
	{
		self->display = display;

		if (xrdp_mm_setup_mod1(self) == 0)
		{
			if (xrdp_mm_setup_mod2(self) == 0)
			{
				xrdp_mm_get_value(self, "ip", ip, 255);
				xrdp_wm_set_login_mode(self->wm, 10);
			}
		}
	}
	else
	{
		log_message(LOG_LEVEL_INFO, "xrdp_mm_process_login_response: "
			"login failed");
	}

	xrdp_mm_cleanup_sesman_connection(self);

	return rv;
}

#ifdef WITH_PAM

int xrdp_mm_access_control(char *username, char *password, char *srv)
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

			Stream_Write_UINT16(out_s, 4); /*0x04 means SCP_GW_AUTHENTICATION*/
			index = g_strlen(username);
			Stream_Write_UINT16(out_s, index);
			Stream_Write(out_s, username, index);

			index = g_strlen(password);
			Stream_Write_UINT16(out_s, index);
			Stream_Write(out_s, password, index);

			index = (int) (out_s->pointer - out_s->buffer);
			out_s->pointer = out_s->buffer;

			Stream_Write_UINT32(out_s, 0); /* version */
			Stream_Write_UINT32(out_s, index); /* size */

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

						Stream_Read_UINT32(in_s, version);
						Stream_Read_UINT32(in_s, size);

						if ((size == 14) && (version == 0))
						{
							Stream_Read_UINT16(in_s, code);
							Stream_Read_UINT16(in_s, pAM_errorcode); /* this variable holds the PAM error code if the variable is >32 it is a "invented" code */
							Stream_Read_UINT16(in_s, dummy);

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

#ifdef WITH_PAM
const char* getPAMError(const int pamError, char *text, int text_bytes)
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

const char* getPAMAdditionalErrorInfo(const int pamError, xrdpMm* self)
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
