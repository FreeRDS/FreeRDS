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
	rdpSettings* settings;

	username = 0;
	password = 0;
	xserverbpp = 0;
	count = self->login_names->count;
	settings = self->wm->session->settings;

	for (index = 0; index < count; index++)
	{
		name = (char*) list_get_item(self->login_names, index);
		value = (char*) list_get_item(self->login_values, index);

		if (g_strcasecmp(name, "username") == 0)
		{
			username = value;
		}
		else if (g_strcasecmp(name, "password") == 0)
		{
			password = value;
		}
		else if (g_strcasecmp(name, "xserverbpp") == 0)
		{
			xserverbpp = g_atoi(value);
		}
	}

	if ((username == 0) || (password == 0))
	{
		return 1;
	}

	s = trans_get_out_s(self->sesman_trans, 8192);
	Stream_Seek(s, 8);

	Stream_Write_UINT16(s, 0);
	index = g_strlen(username);
	Stream_Write_UINT16(s, index);
	Stream_Write(s, username, index);
	index = g_strlen(password);

	Stream_Write_UINT16(s, index);
	Stream_Write(s, password, index);
	Stream_Write_UINT16(s, settings->DesktopWidth);
	Stream_Write_UINT16(s, settings->DesktopHeight);

	if (xserverbpp > 0)
	{
		Stream_Write_UINT16(s, xserverbpp);
	}
	else
	{
		Stream_Write_UINT16(s, settings->ColorDepth);
	}

	/* send domain */
	if (settings->Domain)
	{
		index = g_strlen(settings->Domain);
		Stream_Write_UINT16(s, index);
		Stream_Write(s, settings->Domain, index);
	}
	else
	{
		Stream_Write_UINT16(s, 0);
	}

	/* send program / shell */
	index = g_strlen(settings->AlternateShell);
	Stream_Write_UINT16(s, index);
	Stream_Write(s, settings->AlternateShell, index);

	/* send directory */
	index = g_strlen(settings->ShellWorkingDirectory);
	Stream_Write_UINT16(s, index);
	Stream_Write(s, settings->ShellWorkingDirectory, index);

	/* send client ip */
	index = g_strlen(settings->ClientAddress);
	Stream_Write_UINT16(s, index);
	Stream_Write(s, settings->ClientAddress, index);

	index = (int) (s->pointer - s->buffer);
	Stream_SetPosition(s, 0);

	/* Version 0 of the protocol to sesman is currently used by XRDP */
	Stream_Write_UINT32(s, 0); /* version */
	Stream_Write_UINT32(s, index); /* size */

	s->pointer = s->buffer + index;

	rv = trans_force_write(self->sesman_trans);

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
			}
		}
	}
	else
	{
		log_message(LOG_LEVEL_INFO, "xrdp_mm_process_login_response: "
			"login failed");

		xrdp_mm_cleanup_sesman_connection(self);
	}

	return rv;
}

