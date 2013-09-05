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
	int pid = 0;
	rdpSettings* settings;
	xrdpWm* self = (xrdpWm*) NULL;

	self = (xrdpWm*) g_malloc(sizeof(xrdpWm), 1);
	self->pro_layer = owner;
	self->session = xrdp_process_get_session(owner);
	settings = self->session->settings;

	self->screen = xrdp_bitmap_create(settings->DesktopWidth, settings->DesktopHeight, settings->ColorDepth, WND_TYPE_SCREEN, self);
	self->screen->wm = self;
	pid = g_getpid();

	self->log = list_create();
	self->log->auto_free = 1;
	self->mm = xrdp_mm_create(self);
	get_keymaps(settings->KeyboardLayout, &(self->keymap));
	xrdp_wm_init(self);

	return self;
}

void xrdp_wm_delete(xrdpWm* self)
{
	if (!self)
		return;

	xrdp_mm_delete(self->mm);
	xrdp_bitmap_delete(self->screen);
	list_delete(self->log);

	free(self);
}

int xrdp_wm_load_static_colors_plus(xrdpWm* self, char* autorun_name)
{
	int fd;
	int index;
	char *val;
	xrdpList *names;
	xrdpList *values;
	char cfg_file[256];

	if (autorun_name != 0)
		autorun_name[0] = 0;

	/* now load them from the globals in xrdp.ini if defined */
	g_snprintf(cfg_file, 255, "%s/xrdp.ini", XRDP_CFG_PATH);
	fd = g_file_open(cfg_file);

	if (fd > 0)
	{
		names = list_create();
		names->auto_free = 1;
		values = list_create();
		values->auto_free = 1;

		if (file_read_section(fd, "globals", names, values) == 0)
		{
			for (index = 0; index < names->count; index++)
			{
				val = (char*) list_get_item(names, index);

				if (val)
				{
					if (g_strcasecmp(val, "autorun") == 0)
					{
						val = (char*) list_get_item(values, index);

						if (autorun_name)
							g_strncpy(autorun_name, val, 255);
					}
					else if (g_strcasecmp(val, "pamerrortxt") == 0)
					{
						val = (char*) list_get_item(values, index);
						g_strncpy(self->pamerrortxt,val,255);
					}
				}
			}
		}

		list_delete(names);
		list_delete(values);
		g_file_close(fd);
	}
	else
	{
		log_message(LOG_LEVEL_ERROR,"xrdp_wm_load_static_colors: Could not read xrdp.ini file %s", cfg_file);
	}

	return 0;
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
	char autorun_name[256];

	xrdp_wm_load_static_colors_plus(self, autorun_name);
	self->screen->bg_color = 0;

	if (self->session->settings->AutoLogonEnabled && (autorun_name[0] != 0))
	{
		g_snprintf(cfg_file, 255, "%s/xrdp.ini", XRDP_CFG_PATH);
		fd = g_file_open(cfg_file); /* xrdp.ini */

		if (fd > 0)
		{
			names = list_create();
			names->auto_free = 1;
			values = list_create();
			values->auto_free = 1;

			if (self->session->settings->Domain)
			{
				strcpy(section_name, self->session->settings->Domain);
			}
			else
			{
				section_name[0] = 0;
			}

			if (section_name[0] == 0)
			{
				if (autorun_name[0] == 0)
				{
					file_read_sections(fd, names);

					for (index = 0; index < names->count; index++)
					{
						q = (char*) list_get_item(names, index);

						if (g_strncasecmp("globals", q, 8) != 0)
						{
							g_strncpy(section_name, q, 255);
							break;
						}
					}
				}
				else
				{
					g_strncpy(section_name, autorun_name, 255);
				}
			}

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

	xrdp_mm_connect(self->mm);

	return 0;
}

int xrdp_wm_mouse_move(xrdpWm* self, int x, int y)
{
	if (!self)
		return 0;

	if (x < 0)
		x = 0;

	if (y < 0)
		y = 0;

	if (x >= self->screen->width)
		x = self->screen->width;

	if (y >= self->screen->height)
		y = self->screen->height;

	if (self->mm->mod)
	{
		if (self->mm->mod->client->Event != 0)
		{
			self->mm->mod->client->Event(self->mm->mod, WM_XRDP_MOUSEMOVE, x, y, 0, 0);
		}
	}

	return 0;
}

int xrdp_wm_mouse_click(xrdpWm* self, int x, int y, int button, int down)
{
	pXrdpClientEvent ClientEvent = NULL;

	if (!self)
		return 0;

	if (x < 0)
		x = 0;

	if (y < 0)
		y = 0;

	if (x >= self->screen->width)
		x = self->screen->width;

	if (y >= self->screen->height)
		y = self->screen->height;

	if (self->mm->mod)
	{
		if (self->mm->mod->client->Event)
			ClientEvent = self->mm->mod->client->Event;
	}

	if (ClientEvent)
	{
		if (button == 1)
		{
			ClientEvent(self->mm->mod, (down) ? WM_XRDP_LBUTTONDOWN : WM_XRDP_LBUTTONUP, x, y, 0, 0);
		}
		else if (button == 2)
		{
			ClientEvent(self->mm->mod, (down) ? WM_XRDP_RBUTTONDOWN : WM_XRDP_RBUTTONUP, x, y, 0, 0);
		}
		else if (button == 3)
		{
			ClientEvent(self->mm->mod, (down) ? WM_XRDP_BUTTON3DOWN : WM_XRDP_BUTTON3UP, x, y, 0, 0);
		}
		else if (button == 4)
		{
			ClientEvent(self->mm->mod, (down) ? WM_XRDP_BUTTON4DOWN : WM_XRDP_BUTTON4UP, x, y, 0, 0);
		}
		else if (button == 5)
		{
			ClientEvent(self->mm->mod, (down) ? WM_XRDP_BUTTON5DOWN : WM_XRDP_BUTTON5UP, x, y, 0, 0);
		}
	}

	return 0;
}

/* happens when client gets focus and sends key modifier info */
int xrdp_wm_key_sync(xrdpWm* self, int device_flags, int key_flags)
{
	self->num_lock = 0;
	self->scroll_lock = 0;
	self->caps_lock = 0;

	if (key_flags & 1)
	{
		self->scroll_lock = 1;
	}

	if (key_flags & 2)
	{
		self->num_lock = 1;
	}

	if (key_flags & 4)
	{
		self->caps_lock = 1;
	}

	if (self->mm->mod != 0)
	{
		if (self->mm->mod->client->Event != 0)
		{
			self->mm->mod->client->Event(self->mm->mod, 17, key_flags, device_flags,
					key_flags, device_flags);
		}
	}

	return 0;
}

int xrdp_wm_process_input_mouse(xrdpWm* self, int device_flags, int x, int y)
{
	DEBUG(("mouse event flags %4.4x x %d y %d", device_flags, x, y));

	if (device_flags & PTR_FLAGS_MOVE) /* 0x0800 */
	{
		xrdp_wm_mouse_move(self, x, y);
	}

	if (device_flags & PTR_FLAGS_BUTTON1) /* 0x1000 */
	{
		if (device_flags & PTR_FLAGS_DOWN) /* 0x8000 */
		{
			xrdp_wm_mouse_click(self, x, y, 1, 1);
		}
		else
		{
			xrdp_wm_mouse_click(self, x, y, 1, 0);
		}
	}

	if (device_flags & PTR_FLAGS_BUTTON2) /* 0x2000 */
	{
		if (device_flags & PTR_FLAGS_DOWN) /* 0x8000 */
		{
			xrdp_wm_mouse_click(self, x, y, 2, 1);
		}
		else
		{
			xrdp_wm_mouse_click(self, x, y, 2, 0);
		}
	}

	if (device_flags & PTR_FLAGS_BUTTON3) /* 0x4000 */
	{
		if (device_flags & PTR_FLAGS_DOWN) /* 0x8000 */
		{
			xrdp_wm_mouse_click(self, x, y, 3, 1);
		}
		else
		{
			xrdp_wm_mouse_click(self, x, y, 3, 0);
		}
	}

	if (device_flags == 0x0280 || device_flags == 0x0278)
	{
		xrdp_wm_mouse_click(self, 0, 0, 4, 0);
	}

	if (device_flags == 0x0380 || device_flags == 0x0388)
	{
		xrdp_wm_mouse_click(self, 0, 0, 5, 0);
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

