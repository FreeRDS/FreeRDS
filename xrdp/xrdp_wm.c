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

xrdpWm* xrdp_wm_create(xrdpProcess* owner)
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

	self->LoginModeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	self->painter = xrdp_painter_create(self, self->session);
	self->cache = xrdp_cache_create(self, self->session);
	self->log = list_create();
	self->log->auto_free = 1;
	self->mm = xrdp_mm_create(self);
	self->default_font = xrdp_font_create(self);
	/* this will use built in keymap or load from file */
	get_keymaps(settings->KeyboardLayout, &(self->keymap));
	xrdp_wm_set_login_mode(self, 0);
	self->target_surface = self->screen;
	self->current_surface_index = 0xFFFF; /* screen */

	return self;
}

void xrdp_wm_delete(xrdpWm* self)
{
	if (!self)
		return;

	xrdp_mm_delete(self->mm);
	xrdp_cache_delete(self->cache);
	xrdp_painter_delete(self->painter);
	xrdp_bitmap_delete(self->screen);
	/* free the log */
	list_delete(self->log);
	/* free default font */
	xrdp_font_delete(self->default_font);
	CloseHandle(self->LoginModeEvent);
	/* free self */
	free(self);
}

int xrdp_wm_set_focused(xrdpWm* self, xrdpBitmap *wnd)
{
	xrdpBitmap *focus_out_control;
	xrdpBitmap *focus_in_control;

	if (!self)
		return 0;

	if (self->focused_window == wnd)
		return 0;

	focus_out_control = 0;
	focus_in_control = 0;

	if (self->focused_window != 0)
	{
		xrdp_bitmap_set_focus(self->focused_window, 0);
		focus_out_control = self->focused_window->focused_control;
	}

	self->focused_window = wnd;

	if (self->focused_window != 0)
	{
		xrdp_bitmap_set_focus(self->focused_window, 1);
		focus_in_control = self->focused_window->focused_control;
	}

	xrdp_bitmap_invalidate(focus_out_control, 0);
	xrdp_bitmap_invalidate(focus_in_control, 0);
	return 0;
}

static int xrdp_wm_get_pixel(unsigned char *data, int x, int y, int width, int bpp)
{
	int start;
	int shift;

	if (bpp == 1)
	{
		width = (width + 7) / 8;
		start = (y * width) + x / 8;
		shift = x % 8;
		return (data[start] & (0x80 >> shift)) != 0;
	}
	else if (bpp == 4)
	{
		width = (width + 1) / 2;
		start = y * width + x / 2;
		shift = x % 2;

		if (shift == 0)
		{
			return (data[start] & 0xf0) >> 4;
		}
		else
		{
			return data[start] & 0x0f;
		}
	}

	return 0;
}

int xrdp_wm_pointer(xrdpWm* self, XRDP_MSG_SET_POINTER* msg)
{
	int bpp;
	xrdpPointerItem pointer_item;

	bpp = msg->xorBpp;

	if (!bpp)
		bpp = 24;

	ZeroMemory(&pointer_item, sizeof(xrdpPointerItem));

	pointer_item.x = msg->xPos;
	pointer_item.y = msg->yPos;
	pointer_item.bpp = bpp;
	CopyMemory(pointer_item.data, msg->xorMaskData, msg->lengthXorMask);
	CopyMemory(pointer_item.mask, msg->andMaskData, msg->lengthAndMask);

	self->screen->pointer = xrdp_cache_add_pointer(self->cache, &pointer_item);

	return 0;
}

/* returns error */
int xrdp_wm_load_pointer(xrdpWm* self, char* file_name, char* data, char* mask, int* x, int* y)
{
	int fd;
	int bpp;
	int w;
	int h;
	int i;
	int j;
	int pixel;
	int palette[16];
	wStream* fs;

	if (!g_file_exist(file_name))
	{
		log_message(LOG_LEVEL_ERROR,"xrdp_wm_load_pointer: error pointer file [%s] does not exist",
				file_name);
		return 1;
	}

	fs = Stream_New(NULL, 8192);

	fd = g_file_open(file_name);

	if (fd < 1)
	{
		log_message(LOG_LEVEL_ERROR,"xrdp_wm_load_pointer: error loading pointer from file [%s]",
				file_name);
		return 1;
	}

	g_file_read(fd, fs->buffer, 8192);
	g_file_close(fd);
	Stream_Seek(fs, 6);
	Stream_Read_UINT8(fs, w);
	Stream_Read_UINT8(fs, h);
	Stream_Seek(fs, 2);
	Stream_Read_UINT16(fs, *x);
	Stream_Read_UINT16(fs, *y);
	Stream_Seek(fs, 22);
	Stream_Read_UINT8(fs, bpp);
	Stream_Seek(fs, 25);

	if (w == 32 && h == 32)
	{
		if (bpp == 1)
		{
			Stream_Read(fs, palette, 8);

			for (i = 0; i < 32; i++)
			{
				for (j = 0; j < 32; j++)
				{
					pixel = palette[xrdp_wm_get_pixel(fs->pointer, j, i, 32, 1)];
					*data = pixel;
					data++;
					*data = pixel >> 8;
					data++;
					*data = pixel >> 16;
					data++;
				}
			}

			Stream_Seek(fs, 128);
		}
		else if (bpp == 4)
		{
			Stream_Read(fs, palette, 64);

			for (i = 0; i < 32; i++)
			{
				for (j = 0; j < 32; j++)
				{
					pixel = palette[xrdp_wm_get_pixel(fs->pointer, j, i, 32, 1)];
					*data = pixel;
					data++;
					*data = pixel >> 8;
					data++;
					*data = pixel >> 16;
					data++;
				}
			}

			Stream_Seek(fs, 512);
		}

		g_memcpy(mask, fs->pointer, 128); /* mask */
	}

	Stream_Free(fs, TRUE);

	return 0;
}

/* convert hex string to int */
unsigned int xrdp_wm_htoi (const char *ptr)
{
	unsigned int value = 0;
	char ch = *ptr;

	while (ch == ' ' || ch == '\t')
	{
		ch = *(++ptr);
	}

	for (;;)
	{
		if (ch >= '0' && ch <= '9')
		{
			value = (value << 4) + (ch - '0');
		}
		else if (ch >= 'A' && ch <= 'F')
		{
			value = (value << 4) + (ch - 'A' + 10);
		}
		else if (ch >= 'a' && ch <= 'f')
		{
			value = (value << 4) + (ch - 'a' + 10);
		}
		else
		{
			return value;
		}

		ch = *(++ptr);
	}

	return value;
}

int xrdp_wm_load_static_colors_plus(xrdpWm* self, char *autorun_name)
{
	int bindex;
	int gindex;
	int rindex;

	int fd;
	int index;
	char *val;
	xrdpList *names;
	xrdpList *values;
	char cfg_file[256];

	if (autorun_name != 0)
	{
		autorun_name[0] = 0;
	}

	/* initialize with defaults */
	self->black      = HCOLOR(self->screen->bpp, 0x000000);
	self->grey       = HCOLOR(self->screen->bpp, 0xc0c0c0);
	self->dark_grey  = HCOLOR(self->screen->bpp, 0x808080);
	self->blue       = HCOLOR(self->screen->bpp, 0x0000ff);
	self->dark_blue  = HCOLOR(self->screen->bpp, 0x00007f);
	self->white      = HCOLOR(self->screen->bpp, 0xffffff);
	self->red        = HCOLOR(self->screen->bpp, 0xff0000);
	self->green      = HCOLOR(self->screen->bpp, 0x00ff00);
	self->background = HCOLOR(self->screen->bpp, 0x000000);

	self->hide_log_window = 1;

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
				val = (char *)list_get_item(names, index);

				if (val != 0)
				{
					if (g_strcasecmp(val, "black") == 0)
					{
						val = (char *)list_get_item(values, index);
						self->black = HCOLOR(self->screen->bpp, xrdp_wm_htoi(val));
					}
					else if (g_strcasecmp(val, "grey") == 0)
					{
						val = (char *)list_get_item(values, index);
						self->grey = HCOLOR(self->screen->bpp, xrdp_wm_htoi(val));
					}
					else if (g_strcasecmp(val, "dark_grey") == 0)
					{
						val = (char *)list_get_item(values, index);
						self->dark_grey = HCOLOR(self->screen->bpp, xrdp_wm_htoi(val));
					}
					else if (g_strcasecmp(val, "blue") == 0)
					{
						val = (char *)list_get_item(values, index);
						self->blue = HCOLOR(self->screen->bpp, xrdp_wm_htoi(val));
					}
					else if (g_strcasecmp(val, "dark_blue") == 0)
					{
						val = (char *)list_get_item(values, index);
						self->dark_blue = HCOLOR(self->screen->bpp, xrdp_wm_htoi(val));
					}
					else if (g_strcasecmp(val, "white") == 0)
					{
						val = (char *)list_get_item(values, index);
						self->white = HCOLOR(self->screen->bpp, xrdp_wm_htoi(val));
					}
					else if (g_strcasecmp(val, "red") == 0)
					{
						val = (char *)list_get_item(values, index);
						self->red = HCOLOR(self->screen->bpp, xrdp_wm_htoi(val));
					}
					else if (g_strcasecmp(val, "green") == 0)
					{
						val = (char *)list_get_item(values, index);
						self->green = HCOLOR(self->screen->bpp, xrdp_wm_htoi(val));
					}
					else if (g_strcasecmp(val, "background") == 0)
					{
						val = (char *)list_get_item(values, index);
						self->background = HCOLOR(self->screen->bpp, xrdp_wm_htoi(val));
					}
					else if (g_strcasecmp(val, "autorun") == 0)
					{
						val = (char *)list_get_item(values, index);

						if (autorun_name != 0)
						{
							g_strncpy(autorun_name, val, 255);
						}
					}
					else if (g_strcasecmp(val, "pamerrortxt") == 0)
					{
						val = (char *)list_get_item(values, index);
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

	if (self->screen->bpp == 8)
	{
		/* rgb332 */
		for (bindex = 0; bindex < 4; bindex++)
		{
			for (gindex = 0; gindex < 8; gindex++)
			{
				for (rindex = 0; rindex < 8; rindex++)
				{
					self->palette[(bindex << 6) | (gindex << 3) | rindex] =
							(((rindex << 5) | (rindex << 2) | (rindex >> 1)) << 16) |
							(((gindex << 5) | (gindex << 2) | (gindex >> 1)) << 8) |
							((bindex << 6) | (bindex << 4) | (bindex << 2) | (bindex));
				}
			}
		}

		libxrdp_send_palette(self->session, self->palette);
	}

	return 0;
}

/* returns error */
int xrdp_wm_load_static_pointers(xrdpWm* self)
{
	xrdpPointerItem pointer_item;
	char file_path[256];

	DEBUG(("sending cursor"));
	g_snprintf(file_path, 255, "%s/cursor1.cur", XRDP_SHARE_PATH);
	g_memset(&pointer_item, 0, sizeof(pointer_item));
	xrdp_wm_load_pointer(self, file_path, pointer_item.data,
			pointer_item.mask, &pointer_item.x, &pointer_item.y);
	xrdp_cache_add_pointer_static(self->cache, &pointer_item, 1);
	DEBUG(("sending cursor"));
	g_snprintf(file_path, 255, "%s/cursor0.cur", XRDP_SHARE_PATH);
	g_memset(&pointer_item, 0, sizeof(pointer_item));
	xrdp_wm_load_pointer(self, file_path, pointer_item.data,
			pointer_item.mask, &pointer_item.x, &pointer_item.y);
	xrdp_cache_add_pointer_static(self->cache, &pointer_item, 0);
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
	xrdp_wm_load_static_pointers(self);
	self->screen->bg_color = self->background;

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
			/* domain names that starts with '_' are reserved for IP/DNS to
			 * simplify for the user in a gateway setup */
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
						q = (char *)list_get_item(names, index);

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
					q = (char *)list_get_item(names, index);
					r = (char *)list_get_item(values, index);

					if (strcmp("password", q) == 0)
					{
						/* if the password has been asked for by the module, use what the
                           client says.
                           if the password has been manually set in the config, use that
                           instead of what the client says. */
						if (g_strncmp("ask", r, 3) == 0)
						{
							r = self->session->settings->Password;
						}
					}
					else if (strcmp("username", q) == 0)
					{
						/* if the username has been asked for by the module, use what the
                           client says.
                           if the username has been manually set in the config, use that
                           instead of what the client says. */
						if (strcmp("ask", r) == 0)
						{
							r = self->session->settings->Username;
						}
					}

					list_add_item(self->mm->login_names, (long) g_strdup(q));
					list_add_item(self->mm->login_values, (long) g_strdup(r));
				}

				xrdp_wm_set_login_mode(self, 2);
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

	xrdp_wm_set_login_mode(self, 2); /* force "connected" mode */

	return 0;
}

/* returns the number for rects visible for an area relative to a drawable */
/* putting the rects in region */
int xrdp_wm_get_vis_region(xrdpWm* self, xrdpBitmap* bitmap, int x, int y, int cx, int cy, xrdpRegion *region, int clip_children)
{
	int i;
	xrdpBitmap *p;
	xrdpRect a;
	xrdpRect b;

	/* area we are drawing */
	MAKERECT(a, bitmap->left + x, bitmap->top + y, cx, cy);
	p = bitmap->parent;

	while (p != 0)
	{
		RECTOFFSET(a, p->left, p->top);
		p = p->parent;
	}

	a.left = MAX(self->screen->left, a.left);
	a.top = MAX(self->screen->top, a.top);
	a.right = MIN(self->screen->left + self->screen->width, a.right);
	a.bottom = MIN(self->screen->top + self->screen->height, a.bottom);
	xrdp_region_add_rect(region, &a);

	if (clip_children)
	{
		/* loop through all windows in z order */
		for (i = 0; i < self->screen->child_list->count; i++)
		{
			p = (xrdpBitmap *)list_get_item(self->screen->child_list, i);

			if (p == bitmap || p == bitmap->parent)
			{
				return 0;
			}

			MAKERECT(b, p->left, p->top, p->width, p->height);
			xrdp_region_subtract_rect(region, &b);
		}
	}

	return 0;
}

/* return the window at x, y on the screen */
static xrdpBitmap* xrdp_wm_at_pos(xrdpBitmap *wnd, int x, int y, xrdpBitmap **wnd1)
{
	int i;
	xrdpBitmap *p;
	xrdpBitmap *q;

	/* loop through all windows in z order */
	for (i = 0; i < wnd->child_list->count; i++)
	{
		p = (xrdpBitmap *)list_get_item(wnd->child_list, i);

		if (x >= p->left && y >= p->top && x < p->left + p->width &&
				y < p->top + p->height)
		{
			if (wnd1 != 0)
			{
				*wnd1 = p;
			}

			q = xrdp_wm_at_pos(p, x - p->left, y - p->top, 0);

			if (q == 0)
			{
				return p;
			}
			else
			{
				return q;
			}
		}
	}

	return 0;
}

static int xrdp_wm_xor_pat(xrdpWm* self, int x, int y, int cx, int cy)
{
	self->painter->clip_children = 0;
	self->painter->rop = 0x5a;
	xrdp_painter_begin_update(self->painter);
	self->painter->use_clip = 0;
	self->painter->brush.pattern[0] = 0xaa;
	self->painter->brush.pattern[1] = 0x55;
	self->painter->brush.pattern[2] = 0xaa;
	self->painter->brush.pattern[3] = 0x55;
	self->painter->brush.pattern[4] = 0xaa;
	self->painter->brush.pattern[5] = 0x55;
	self->painter->brush.pattern[6] = 0xaa;
	self->painter->brush.pattern[7] = 0x55;
	self->painter->brush.x_orgin = 0;
	self->painter->brush.x_orgin = 0;
	self->painter->brush.style = 3;
	self->painter->bg_color = self->black;
	self->painter->fg_color = self->white;
	xrdp_painter_patblt(self->painter, self->screen, x, y, cx, 5); /* top */
	xrdp_painter_patblt(self->painter, self->screen, x, y + (cy - 5), cx, 5); /* bottom */
	xrdp_painter_patblt(self->painter, self->screen, x, y + 5, 5, cy - 10); /* left */
	xrdp_painter_patblt(self->painter, self->screen, x + (cx - 5), y + 5, 5, cy - 10); /* right */
	xrdp_painter_end_update(self->painter);
	self->painter->rop = 0xCC;
	self->painter->clip_children = 1;
	return 0;
}

/* this don't are about nothing, just copy the bits */
/* no clipping rects, no windows in the way, nothing */
static int xrdp_wm_bitblt(xrdpWm* self, xrdpBitmap *dst, int dx, int dy,
		xrdpBitmap *src, int sx, int sy, int sw, int sh, int rop)
{
	if (self->screen == dst && self->screen == src)
	{
		libxrdp_orders_init(self->session);
		libxrdp_orders_screen_blt(self->session, dx, dy, sw, sh, sx, sy, rop, 0);
		libxrdp_orders_send(self->session);
	}

	return 0;
}

/* return true is rect is totaly exposed going in reverse z order */
/* from wnd up */
static int xrdp_wm_is_rect_vis(xrdpWm* self, xrdpBitmap *wnd, xrdpRect *rect)
{
	xrdpRect wnd_rect;
	xrdpBitmap *b;
	int i;;

	/* if rect is part off screen */
	if (rect->left < 0)
		return 0;

	if (rect->top < 0)
		return 0;

	if (rect->right >= self->screen->width)
		return 0;

	if (rect->bottom >= self->screen->height)
		return 0;

	i = list_index_of(self->screen->child_list, (long)wnd);
	i--;

	while (i >= 0)
	{
		b = (xrdpBitmap *)list_get_item(self->screen->child_list, i);
		MAKERECT(wnd_rect, b->left, b->top, b->width, b->height);

		if (rect_intersect(rect, &wnd_rect, 0))
		{
			return 0;
		}

		i--;
	}

	return 1;
}

static int xrdp_wm_move_window(xrdpWm* self, xrdpBitmap *wnd, int dx, int dy)
{
	xrdpRect rect1;
	xrdpRect rect2;
	xrdpRegion *r;
	int i;

	MAKERECT(rect1, wnd->left, wnd->top, wnd->width, wnd->height);

	if (xrdp_wm_is_rect_vis(self, wnd, &rect1))
	{
		rect2 = rect1;
		RECTOFFSET(rect2, dx, dy);

		if (xrdp_wm_is_rect_vis(self, wnd, &rect2))
		{
			/* if both src and dst are unobscured, we can do a bitblt move */
			xrdp_wm_bitblt(self, self->screen, wnd->left + dx, wnd->top + dy,
					self->screen, wnd->left, wnd->top,
					wnd->width, wnd->height, 0xcc);
			wnd->left += dx;
			wnd->top += dy;
			r = xrdp_region_create(self);
			xrdp_region_add_rect(r, &rect1);
			xrdp_region_subtract_rect(r, &rect2);
			i = 0;

			while (xrdp_region_get_rect(r, i, &rect1) == 0)
			{
				xrdp_bitmap_invalidate(self->screen, &rect1);
				i++;
			}

			xrdp_region_delete(r);
			return 0;
		}
	}

	wnd->left += dx;
	wnd->top += dy;
	xrdp_bitmap_invalidate(self->screen, &rect1);
	xrdp_bitmap_invalidate(wnd, 0);
	return 0;
}

static int xrdp_wm_undraw_dragging_box(xrdpWm* self, int do_begin_end)
{
	int boxx;
	int boxy;

	if (!self)
		return 0;

	if (self->dragging)
	{
		if (self->draggingxorstate)
		{
			if (do_begin_end)
			{
				xrdp_painter_begin_update(self->painter);
			}

			boxx = self->draggingx - self->draggingdx;
			boxy = self->draggingy - self->draggingdy;
			xrdp_wm_xor_pat(self, boxx, boxy, self->draggingcx, self->draggingcy);
			self->draggingxorstate = 0;

			if (do_begin_end)
			{
				xrdp_painter_end_update(self->painter);
			}
		}
	}

	return 0;
}

static int xrdp_wm_draw_dragging_box(xrdpWm* self, int do_begin_end)
{
	int boxx;
	int boxy;

	if (!self)
		return 0;

	if (self->dragging)
	{
		if (!self->draggingxorstate)
		{
			if (do_begin_end)
			{
				xrdp_painter_begin_update(self->painter);
			}

			boxx = self->draggingx - self->draggingdx;
			boxy = self->draggingy - self->draggingdy;
			xrdp_wm_xor_pat(self, boxx, boxy, self->draggingcx, self->draggingcy);
			self->draggingxorstate = 1;

			if (do_begin_end)
			{
				xrdp_painter_end_update(self->painter);
			}
		}
	}

	return 0;
}

int xrdp_wm_mouse_move(xrdpWm* self, int x, int y)
{
	xrdpBitmap *b;

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

	self->mouse_x = x;
	self->mouse_y = y;

	if (self->dragging)
	{
		xrdp_painter_begin_update(self->painter);
		xrdp_wm_undraw_dragging_box(self, 0);
		self->draggingx = x;
		self->draggingy = y;
		xrdp_wm_draw_dragging_box(self, 0);
		xrdp_painter_end_update(self->painter);
		return 0;
	}

	b = xrdp_wm_at_pos(self->screen, x, y, 0);

	if (b == 0) /* if b is null, the movement must be over the screen */
	{
		if (self->screen->pointer != self->current_pointer)
		{
			libxrdp_set_pointer(self->session, self->screen->pointer);
			self->current_pointer = self->screen->pointer;
		}

		if (self->mm->mod != 0) /* if screen is mod controlled */
		{
			if (self->mm->mod->client->Event != 0)
			{
				self->mm->mod->client->Event(self->mm->mod, WM_XRDP_MOUSEMOVE, x, y, 0, 0);
			}
		}
	}

	if (self->button_down != 0)
	{
		if (b == self->button_down && self->button_down->state == 0)
		{
			self->button_down->state = 1;
			xrdp_bitmap_invalidate(self->button_down, 0);
		}
		else if (b != self->button_down)
		{
			self->button_down->state = 0;
			xrdp_bitmap_invalidate(self->button_down, 0);
		}
	}

	if (b != 0)
	{
		if (!self->dragging)
		{
			if (b->pointer != self->current_pointer)
			{
				libxrdp_set_pointer(self->session, b->pointer);
				self->current_pointer = b->pointer;
			}

			xrdp_bitmap_def_proc(b, WM_XRDP_MOUSEMOVE,
					xrdp_bitmap_from_screenx(b, x),
					xrdp_bitmap_from_screeny(b, y));

			if (self->button_down == 0)
			{
				if (b->notify != 0)
				{
					b->notify(b->owner, b, 2, x, y);
				}
			}
		}
	}

	return 0;
}

int xrdp_wm_mouse_click(xrdpWm* self, int x, int y, int but, int down)
{
	xrdpBitmap *control;
	xrdpBitmap *focus_out_control;
	xrdpBitmap *wnd;
	int newx;
	int newy;
	int oldx;
	int oldy;

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

	if (self->dragging && but == 1 && !down && self->dragging_window != 0)
	{
		/* if done dragging */
		self->draggingx = x;
		self->draggingy = y;
		newx = self->draggingx - self->draggingdx;
		newy = self->draggingy - self->draggingdy;
		oldx = self->dragging_window->left;
		oldy = self->dragging_window->top;

		/* draw xor box one more time */
		if (self->draggingxorstate)
		{
			xrdp_wm_xor_pat(self, newx, newy, self->draggingcx, self->draggingcy);
		}

		self->draggingxorstate = 0;
		/* move screen to new location */
		xrdp_wm_move_window(self, self->dragging_window, newx - oldx, newy - oldy);
		self->dragging_window = 0;
		self->dragging = 0;
	}

	xrdp_wm_set_focused(self, 0);

	/* no matter what, mouse is up, reset button_down */
	if (but == 1 && !down && self->button_down != 0)
	{
		self->button_down = 0;
	}

	return 0;
}

int xrdp_wm_key(xrdpWm* self, int device_flags, int scan_code)
{
	int msg;
	xrdpKeyInfo *ki;

	/*g_printf("count %d\n", self->key_down_list->count);*/
	scan_code = scan_code % 128;

	if (device_flags & KBD_FLAGS_RELEASE) /* 0x8000 */
	{
		self->keys[scan_code] = 0;
		msg = WM_XRDP_KEYUP;
	}
	else /* key down */
	{
		self->keys[scan_code] = 1 | device_flags;
		msg = WM_XRDP_KEYDOWN;

		switch (scan_code)
		{
			case 58:
				self->caps_lock = !self->caps_lock;
				break; /* caps lock */
			case 69:
				self->num_lock = !self->num_lock;
				break; /* num lock */
			case 70:
				self->scroll_lock = !self->scroll_lock;
				break; /* scroll lock */
		}
	}

	if (self->mm->mod != 0)
	{
		if (self->mm->mod->client->Event != 0)
		{
			ki = get_key_info_from_scan_code(device_flags, scan_code, self->keys, self->caps_lock,
							self->num_lock, self->scroll_lock, &(self->keymap));

			if (ki != 0)
			{
				self->mm->mod->client->Event(self->mm->mod, msg, ki->chr, ki->sym,
						scan_code, device_flags);
			}
		}
	}
	else if (self->focused_window != 0)
	{
		xrdp_bitmap_def_proc(self->focused_window, msg, scan_code, device_flags);
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

int xrdp_wm_delete_all_childs(xrdpWm* self)
{
	int index;
	xrdpBitmap *b;
	xrdpRect rect;

	for (index = self->screen->child_list->count - 1; index >= 0; index--)
	{
		b = (xrdpBitmap*) list_get_item(self->screen->child_list, index);
		MAKERECT(rect, b->left, b->top, b->width, b->height);
		xrdp_bitmap_delete(b);
		xrdp_bitmap_invalidate(self->screen, &rect);
	}

	return 0;
}

/* this is the callbacks coming from libxrdp.so */
int callback(long id, int msg, long param1, long param2, long param3, long param4)
{
	int rv;
	xrdpWm *wm;
	xrdpRect rect;

	if (!id)
	{
		/* "id" should be "xrdpProcess*" as long */
		return 0;
	}

	wm = xrdp_process_get_wm((xrdpProcess*) id);

	if (!wm)
		return 0;

	rv = 0;

	switch (msg)
	{
		case 0: /* RDP_INPUT_SYNCHRONIZE */
			rv = xrdp_wm_key_sync(wm, param3, param1);
			break;
		case 4: /* RDP_INPUT_SCANCODE */
			rv = xrdp_wm_key(wm, param3, param1);
			break;
		case 0x8001: /* RDP_INPUT_MOUSE */
			rv = xrdp_wm_process_input_mouse(wm, param3, param1, param2);
			break;
		case 0x4444: /* invalidate, this is not from RDP_DATA_PDU_INPUT */
			/* like the rest, its from RDP_PDU_DATA with code 33 */
			/* its the rdp client asking for a screen update */
			MAKERECT(rect, param1, param2, param3, param4);
			rv = xrdp_bitmap_invalidate(wm->screen, &rect);
			break;
	}

	return rv;
}

/* returns error */
/* this gets called when there is nothing on any socket */
static int xrdp_wm_login_mode_changed(xrdpWm* self)
{
	if (!self)
		return 0;

	if (self->login_mode == 0)
	{
		/* this is the initial state of the login window */
		xrdp_wm_set_login_mode(self, 1); /* put the wm in login mode */
		list_clear(self->log);
		xrdp_wm_delete_all_childs(self);
		self->dragging = 0;
		xrdp_wm_init(self);
	}
	else if (self->login_mode == 2)
	{
		if (xrdp_mm_connect(self->mm) == 0)
		{
			xrdp_wm_set_login_mode(self, 3); /* put the wm in connected mode */
			xrdp_wm_delete_all_childs(self);
			self->dragging = 0;
		}
		else
		{
			/* we do nothing on connect error so far */
		}
	}
	else if (self->login_mode == 10)
	{
		xrdp_wm_delete_all_childs(self);
		self->dragging = 0;
		xrdp_wm_set_login_mode(self, 11);
	}

	return 0;
}

int xrdp_wm_set_login_mode(xrdpWm* self, int login_mode)
{
	self->login_mode = login_mode;
	SetEvent(self->LoginModeEvent);
	return 0;
}

int xrdp_wm_get_event_handles(xrdpWm* self, HANDLE* events, DWORD* nCount)
{
	if (!self)
		return 0;

	events[*nCount] = self->LoginModeEvent;
	(*nCount)++;

	return xrdp_mm_get_event_handles(self->mm, events, nCount);
}

int xrdp_wm_check_wait_objs(xrdpWm* self)
{
	int status;

	if (!self)
		return 0;

	status = 0;

	if (WaitForSingleObject(self->LoginModeEvent, 0) == WAIT_OBJECT_0)
	{
		ResetEvent(self->LoginModeEvent);
		xrdp_wm_login_mode_changed(self);
	}

	if (status == 0)
	{
		status = xrdp_mm_check_wait_objs(self->mm);
	}

	return status;
}

