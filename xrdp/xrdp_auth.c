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

	settings = self->wm->session->settings;

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

		if (xrdp_mm_setup_mod1(self) == 0)
		{
			if (xrdp_mm_setup_mod2(self) == 0)
			{

			}
		}
	}
	else
	{
		log_message(LOG_LEVEL_INFO, "xrdp_mm_process_login_response: "
			"login failed");

		xrdp_mm_cleanup_sesman_connection(self);
	}

	return status;
}

