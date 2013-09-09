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
 * simple window manager
 */

#include "xrdp.h"
#include "log.h"

xrdpWm* xrdp_wm_create(xrdpSession* owner)
{
	rdpSettings* settings;
	xrdpWm* self = (xrdpWm*) NULL;

	self = (xrdpWm*) g_malloc(sizeof(xrdpWm), 1);
	self->pro_layer = owner;
	self->session = xrdp_process_get_session(owner);
	settings = self->session->settings;

	self->log = list_create();
	self->log->auto_free = 1;
	self->mm = xrdp_mm_create(self);
	xrdp_wm_init(self);
	xrdp_mm_connect(self->mm);

	return self;
}

void xrdp_wm_delete(xrdpWm* self)
{
	if (!self)
		return;

	xrdp_mm_delete(self->mm);
	list_delete(self->log);

	free(self);
}

int xrdp_wm_init(xrdpWm* self)
{
	int fd;
	int index;
	xrdpList *names;
	xrdpList *values;
	char *q;
	char *r;
	char section_name[256];
	char cfg_file[256];

	if (self->session->settings->AutoLogonEnabled)
	{
		g_snprintf(cfg_file, 255, "%s/xrdp.ini", XRDP_CFG_PATH);
		fd = g_file_open(cfg_file); /* xrdp.ini */

		if (fd > 0)
		{
			names = list_create();
			names->auto_free = 1;
			values = list_create();
			values->auto_free = 1;

			strcpy(section_name, "xrdp1");

			list_clear(names);

			if (file_read_section(fd, section_name, names, values) == 0)
			{
				for (index = 0; index < names->count; index++)
				{
					q = (char*) list_get_item(names, index);
					r = (char*) list_get_item(values, index);

					if (strcmp("password", q) == 0)
					{
						if (g_strncmp("ask", r, 3) == 0)
						{
							r = self->session->settings->Password;
						}
					}
					else if (strcmp("username", q) == 0)
					{
						if (strcmp("ask", r) == 0)
						{
							r = self->session->settings->Username;
						}
					}

					list_add_item(self->mm->login_names, (long) g_strdup(q));
					list_add_item(self->mm->login_values, (long) g_strdup(r));
				}
			}

			list_delete(names);
			list_delete(values);
			g_file_close(fd);
		}
		else
		{
			log_message(LOG_LEVEL_ERROR,"xrdp_wm_init: Could not read xrdp.ini file %s", cfg_file);
		}
	}

	return 0;
}

int xrdp_wm_get_event_handles(xrdpWm* self, HANDLE* events, DWORD* nCount)
{
	if (!self)
		return 0;

	return xrdp_mm_get_event_handles(self->mm, events, nCount);
}

int xrdp_wm_check_wait_objs(xrdpWm* self)
{
	int status;

	if (!self)
		return 0;

	status = xrdp_mm_check_wait_objs(self->mm);

	return status;
}

