/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2012
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
 * painter, gc
 */

#include "xrdp.h"

xrdpPainter* xrdp_painter_create(xrdpWm *wm, xrdpSession *session)
{
	xrdpPainter* self;

	self = (xrdpPainter *) g_malloc(sizeof(xrdpPainter), 1);
	self->wm = wm;
	self->session = session;
	self->rop = 0xCC;
	self->clip_children = 1;

	return self;
}

void xrdp_painter_delete(xrdpPainter* self)
{
	if (!self)
		return;

	free(self);
}

int wm_painter_set_target(xrdpPainter* self)
{
	int surface_index;
	int index;
	xrdpList *del_list;

	if (self->wm->target_surface->type == WND_TYPE_SCREEN)
	{
		if (self->wm->current_surface_index != 0xFFFF)
		{
			libxrdp_orders_send_switch_os_surface(self->session, 0xFFFF);
			self->wm->current_surface_index = 0xFFFF;
		}
	}
	else if (self->wm->target_surface->type == WND_TYPE_OFFSCREEN)
	{
		surface_index = self->wm->target_surface->item_index;

		if (surface_index != self->wm->current_surface_index)
		{
			if (self->wm->target_surface->tab_stop == 0) /* tab_stop is hack */
			{
				del_list = self->wm->cache->xrdp_os_del_list;
				index = list_index_of(del_list, surface_index);
				list_remove_item(del_list, index);
				libxrdp_orders_send_create_os_surface(self->session, surface_index,
						self->wm->target_surface->width,
						self->wm->target_surface->height, del_list);
				self->wm->target_surface->tab_stop = 1;
				list_clear(del_list);
			}

			libxrdp_orders_send_switch_os_surface(self->session, surface_index);
			self->wm->current_surface_index = surface_index;
		}
	}
	else
	{
		g_writeln("xrdp_painter_begin_update: bad target_surface");
	}

	return 0;
}

int xrdp_painter_begin_update(xrdpPainter* self)
{
	if (!self)
		return 0;

	libxrdp_orders_init(self->session);
	wm_painter_set_target(self);
	return 0;
}

int xrdp_painter_end_update(xrdpPainter* self)
{
	if (!self)
		return 0;

	libxrdp_orders_send(self->session);
	return 0;
}

int xrdp_painter_font_needed(xrdpPainter* self)
{
	if (!self->font)
		self->font = self->wm->default_font;

	return 0;
}

int xrdp_painter_set_clip(xrdpPainter* self, int x, int y, int cx, int cy)
{
	self->use_clip = &self->clip;
	self->clip.left = x;
	self->clip.top = y;
	self->clip.right = x + cx;
	self->clip.bottom = y + cy;
	return 0;
}

int xrdp_painter_clr_clip(xrdpPainter* self)
{
	self->use_clip = 0;
	return 0;
}

int xrdp_painter_text_width(xrdpPainter* self, char *text)
{
	int index;
	int rv;
	int len;
	xrdpFontChar *font_item;
	wchar_t *wstr;

	xrdp_painter_font_needed(self);

	if (!self->font)
		return 0;

	if (!text)
		return 0;

	rv = 0;
	len = g_mbstowcs(0, text, 0);
	wstr = (wchar_t *) g_malloc((len + 2) * sizeof(wchar_t), 0);
	g_mbstowcs(wstr, text, len + 1);

	for (index = 0; index < len; index++)
	{
		font_item = self->font->font_items + wstr[index];
		rv = rv + font_item->incby;
	}

	free(wstr);
	return rv;
}

int xrdp_painter_text_height(xrdpPainter* self, char *text)
{
	int index;
	int rv;
	int len;
	xrdpFontChar *font_item;
	wchar_t *wstr;

	xrdp_painter_font_needed(self);

	if (!self->font)
		return 0;

	if (!text)
		return 0;

	rv = 0;
	len = g_mbstowcs(0, text, 0);
	wstr = (wchar_t *) g_malloc((len + 2) * sizeof(wchar_t), 0);
	g_mbstowcs(wstr, text, len + 1);

	for (index = 0; index < len; index++)
	{
		font_item = self->font->font_items + wstr[index];
		rv = MAX(rv, font_item->height);
	}

	free(wstr);
	return rv;
}

static int xrdp_painter_setup_brush(xrdpPainter* self, xrdpBrush *out_brush, xrdpBrush *in_brush)
{
	int cache_id;

	g_memcpy(out_brush, in_brush, sizeof(xrdpBrush));

	if (in_brush->style == 3)
	{
		if (self->session->settings->BrushSupportLevel == 1)
		{
			cache_id = xrdp_cache_add_brush(self->wm->cache, in_brush->pattern);
			g_memset(out_brush->pattern, 0, 8);
			out_brush->pattern[0] = cache_id;
			out_brush->style = 0x81;
		}
	}

	return 0;
}

/* fill in an area of the screen with one color */
int xrdp_painter_fill_rect(xrdpPainter* self, xrdpBitmap* dst, int x, int y, int cx, int cy)
{
	int k;
	int dx;
	int dy;
	xrdpRect clip_rect;
	xrdpRect draw_rect;
	xrdpRect rect;
	xrdpRegion* region;

	if (!self)
		return 0;

	if (dst->type == WND_TYPE_BITMAP)
		return 0;

	xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
	region = xrdp_region_create(self->wm);

	if (dst->type != WND_TYPE_OFFSCREEN)
		xrdp_wm_get_vis_region(self->wm, dst, x, y, cx, cy, region, self->clip_children);
	else
		xrdp_region_add_rect(region, &clip_rect);

	x += dx;
	y += dy;

	if (self->rop == 0xCC)
	{
		k = 0;

		while (xrdp_region_get_rect(region, k, &rect) == 0)
		{
			if (rect_intersect(&rect, &clip_rect, &draw_rect))
			{
				libxrdp_orders_rect(self->session, x, y, cx, cy, self->fg_color, &draw_rect);
			}

			k++;
		}
	}
	else if (((self->rop & 0xf) == 0x0 || /* black */
		(self->rop & 0xf) == 0xf || /* white */
		(self->rop & 0xf) == 0x5)) /* DSTINVERT */
	{
		k = 0;

		while (xrdp_region_get_rect(region, k, &rect) == 0)
		{
			if (rect_intersect(&rect, &clip_rect, &draw_rect))
			{
				libxrdp_orders_dest_blt(self->session, x, y, cx, cy, self->rop, &draw_rect);
			}

			k++;
		}
	}

	xrdp_region_delete(region);
	return 0;
}

int xrdp_painter_patblt(xrdpPainter* self, xrdpBitmap* dst, int x, int y, int cx, int cy)
{
	int k;
	int dx;
	int dy;
	int rop;
	xrdpRect rect;
	xrdpBrush brush;
	xrdpRect clip_rect;
	xrdpRect draw_rect;
	xrdpRegion* region;

	if (!self)
		return 0;

	if (dst->type == WND_TYPE_BITMAP)
		return 0;

	xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
	region = xrdp_region_create(self->wm);

	if (dst->type != WND_TYPE_OFFSCREEN)
		xrdp_wm_get_vis_region(self->wm, dst, x, y, cx, cy, region, self->clip_children);
	else
		xrdp_region_add_rect(region, &clip_rect);

	x += dx;
	y += dy;

	k = 0;
	rop = self->rop;

	/* if opcode is in the form 0x00, 0x11, 0x22, ... convert it */
	if (((rop & 0xF0) >> 4) == (rop & 0xf))
	{
		switch (rop)
		{
			case 0x66: /* xor */
				rop = 0x5a;
				break;
			case 0xaa: /* noop */
				rop = 0xfb;
				break;
			case 0xcc: /* copy */
				rop = 0xf0;
				break;
			case 0x88: /* and */
				rop = 0xc0;
				break;
		}
	}

	xrdp_painter_setup_brush(self, &brush, &self->brush);

	while (xrdp_region_get_rect(region, k, &rect) == 0)
	{
		if (rect_intersect(&rect, &clip_rect, &draw_rect))
		{
			libxrdp_orders_pat_blt(self->session, x, y, cx, cy, rop, self->bg_color,
					self->fg_color, &brush, &draw_rect);
		}

		k++;
	}

	xrdp_region_delete(region);

	return 0;
}

int xrdp_painter_draw_text(xrdpPainter* self, xrdpBitmap *dst, int x, int y, const char *text)
{
	int i;
	int f;
	int c;
	int k;
	int x1;
	int y1;
	int flags;
	int len;
	int index;
	int total_width;
	int total_height;
	int dx;
	int dy;
	char* data;
	xrdpRegion* region;
	xrdpRect rect;
	xrdpRect clip_rect;
	xrdpRect draw_rect;
	xrdpFont* font;
	xrdpFontChar* font_item;
	wchar_t* wstr;
	GLYPH_INDEX_ORDER msg;

	if (!self)
		return 0;

	len = g_mbstowcs(0, text, 0);

	if (len < 1)
		return 0;

	/* todo data */

	if (dst->type == 0)
		return 0;

	xrdp_painter_font_needed(self);

	if (self->font == 0)
		return 0;

	/* convert to wide char */
	wstr = (wchar_t *) g_malloc((len + 2) * sizeof(wchar_t), 0);
	g_mbstowcs(wstr, text, len + 1);
	font = self->font;
	f = 0;
	k = 0;
	total_width = 0;
	total_height = 0;
	data = (char*) g_malloc(len * 4, 1);

	for (index = 0; index < len; index++)
	{
		font_item = font->font_items + wstr[index];
		i = xrdp_cache_add_char(self->wm->cache, font_item);
		f = HIWORD(i);
		c = LOWORD(i);
		data[index * 2] = c;
		data[index * 2 + 1] = k;
		k = font_item->incby;
		total_width += k;
		total_height = MAX(total_height, font_item->height);
	}

	xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
	region = xrdp_region_create(self->wm);

	if (dst->type != WND_TYPE_OFFSCREEN)
	{
		xrdp_wm_get_vis_region(self->wm, dst, x, y, total_width, total_height, region, self->clip_children);
	}
	else
	{
		xrdp_region_add_rect(region, &clip_rect);
	}

	x += dx;
	y += dy;
	k = 0;

	while (xrdp_region_get_rect(region, k, &rect) == 0)
	{
		if (rect_intersect(&rect, &clip_rect, &draw_rect))
		{
			x1 = x;
			y1 = y + total_height;
			flags = 0x03; /* 0x03 0x73; TEXT2_IMPLICIT_X and something else */

			msg.cacheId = (UINT32) f;
			msg.flAccel = flags;
			msg.ulCharInc = 0;
			msg.fOpRedundant = 0;
			msg.backColor = 0;
			msg.foreColor = self->fg_color;
			msg.bkLeft = x - 1;
			msg.bkTop = y - 1;
			msg.bkRight = x + total_width;
			msg.bkBottom = y + total_height;
			msg.opLeft = 0;
			msg.opTop = 0;
			msg.opRight = 0;
			msg.opBottom = 0;
			msg.x = x1;
			msg.y = y1;
			msg.cbData = len * 2;
			CopyMemory(msg.data, data, msg.cbData);

			libxrdp_orders_text(self->session, &msg, &draw_rect);
		}

		k++;
	}

	xrdp_region_delete(region);
	free(data);
	free(wstr);

	return 0;
}

int xrdp_painter_draw_text2(xrdpPainter* self, xrdpBitmap* dst, GLYPH_INDEX_ORDER* msg)
{
	xrdpRect clip_rect;
	xrdpRect draw_rect;
	xrdpRect rect;
	xrdpRegion* region;
	int k;
	int dx;
	int dy;

	if (!self)
		return 0;

	/* todo data */

	if (dst->type == WND_TYPE_BITMAP)
		return 0;

	xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
	region = xrdp_region_create(self->wm);

	if (dst->type != WND_TYPE_OFFSCREEN)
	{
		if (msg->opRight - msg->opLeft > 1)
		{
			xrdp_wm_get_vis_region(self->wm, dst, msg->opLeft, msg->opTop,
					msg->opRight - msg->opLeft, msg->opBottom - msg->opTop,
					region, self->clip_children);
		}
		else
		{
			xrdp_wm_get_vis_region(self->wm, dst, msg->opLeft, msg->opTop,
					msg->opRight - msg->opLeft, msg->opBottom - msg->opTop,
					region, self->clip_children);
		}
	}
	else
	{
		xrdp_region_add_rect(region, &clip_rect);
	}

	msg->bkLeft += dx;
	msg->bkTop += dy;
	msg->bkRight += dx;
	msg->bkBottom += dy;
	msg->opLeft += dx;
	msg->opTop += dy;
	msg->opRight += dx;
	msg->opBottom += dy;
	msg->x += dx;
	msg->y += dy;
	k = 0;

	while (xrdp_region_get_rect(region, k, &rect) == 0)
	{
		if (rect_intersect(&rect, &clip_rect, &draw_rect))
		{
			libxrdp_orders_text(self->session, msg, &draw_rect);
		}

		k++;
	}

	xrdp_region_delete(region);

	return 0;
}

int xrdp_painter_copy(xrdpPainter* self, xrdpBitmap *src, xrdpBitmap *dst, int x, int y, int cx, int cy, int srcx, int srcy)
{
	xrdpRect clip_rect;
	xrdpRect draw_rect;
	xrdpRect rect1;
	xrdpRect rect2;
	xrdpRegion *region;
	xrdpBitmap *b;
	int i;
	int j;
	int k;
	int dx;
	int dy;
	int palette_id;
	int bitmap_id;
	int cache_id;
	int cache_idx;
	int dstx;
	int dsty;
	int w;
	int h;
	int index;
	xrdpList *del_list;

	if (!self || !src || !dst)
		return 0;

	/* todo data */

	if (dst->type == WND_TYPE_BITMAP)
		return 0;

	if (src->type == WND_TYPE_SCREEN)
	{
		xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
		region = xrdp_region_create(self->wm);

		if (dst->type != WND_TYPE_OFFSCREEN)
		{
			xrdp_wm_get_vis_region(self->wm, dst, x, y, cx, cy, region, self->clip_children);
		}
		else
		{
			xrdp_region_add_rect(region, &clip_rect);
		}

		x += dx;
		y += dy;
		srcx += dx;
		srcy += dy;
		k = 0;

		while (xrdp_region_get_rect(region, k, &rect1) == 0)
		{
			if (rect_intersect(&rect1, &clip_rect, &draw_rect))
			{
				libxrdp_orders_screen_blt(self->session, x, y, cx, cy, srcx, srcy, self->rop,
						&draw_rect);
			}

			k++;
		}

		xrdp_region_delete(region);
	}
	else if (src->type == WND_TYPE_OFFSCREEN)
	{
		g_writeln("xrdp_painter_copy: todo");

		xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
		region = xrdp_region_create(self->wm);

		if (dst->type != WND_TYPE_OFFSCREEN)
		{
			g_writeln("off screen to screen");
			xrdp_wm_get_vis_region(self->wm, dst, x, y, cx, cy, region, self->clip_children);
		}
		else
		{
			g_writeln("off screen to off screen");
			xrdp_region_add_rect(region, &clip_rect);
		}

		x += dx;
		y += dy;

		palette_id = 0;
		cache_id = 255; // todo
		cache_idx = src->item_index; // todo

		if (src->tab_stop == 0)
		{
			g_writeln("xrdp_painter_copy: warning src not created");
			del_list = self->wm->cache->xrdp_os_del_list;
			index = list_index_of(del_list, cache_idx);
			list_remove_item(del_list, index);
			libxrdp_orders_send_create_os_surface(self->session, cache_idx, src->width, src->height, del_list);
			src->tab_stop = 1;
			list_clear(del_list);
		}

		k = 0;

		while (xrdp_region_get_rect(region, k, &rect1) == 0)
		{
			if (rect_intersect(&rect1, &clip_rect, &rect2))
			{
				MAKERECT(rect1, x, y, cx, cy);

				if (rect_intersect(&rect2, &rect1, &draw_rect))
				{
					libxrdp_orders_mem_blt(self->session, cache_id, palette_id, x, y, cx,
							cy, self->rop, srcx, srcy, cache_idx, &draw_rect);
				}
			}

			k++;
		}

		xrdp_region_delete(region);
	}
	else if (src->data != 0)
	{
		/* todo, the non bitmap cache part is gone, it should be put back */

		xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
		region = xrdp_region_create(self->wm);

		if (dst->type != WND_TYPE_OFFSCREEN)
		{
			xrdp_wm_get_vis_region(self->wm, dst, x, y, cx, cy, region, self->clip_children);
		}
		else
		{
			xrdp_region_add_rect(region, &clip_rect);
		}

		x += dx;
		y += dy;
		palette_id = 0;
		j = srcy;

		while (j < (srcy + cy))
		{
			i = srcx;

			while (i < (srcx + cx))
			{
				w = MIN(64, ((srcx + cx) - i));
				h = MIN(64, ((srcy + cy) - j));
				b = xrdp_bitmap_create(w, h, self->wm->screen->bpp, 0, self->wm);
				xrdp_bitmap_copy_box_with_crc(src, b, i, j, w, h);
				bitmap_id = xrdp_cache_add_bitmap(self->wm->cache, b, self->wm->hints);
				cache_id = HIWORD(bitmap_id);
				cache_idx = LOWORD(bitmap_id);
				dstx = (x + i) - srcx;
				dsty = (y + j) - srcy;
				k = 0;

				while (xrdp_region_get_rect(region, k, &rect1) == 0)
				{
					if (rect_intersect(&rect1, &clip_rect, &rect2))
					{
						MAKERECT(rect1, dstx, dsty, w, h);

						if (rect_intersect(&rect2, &rect1, &draw_rect))
						{
							libxrdp_orders_mem_blt(self->session, cache_id,
									palette_id, dstx, dsty, w, h,
									self->rop, 0, 0, cache_idx,
									&draw_rect);
						}
					}

					k++;
				}

				i += 64;
			}

			j += 64;
		}

		xrdp_region_delete(region);
	}

	return 0;
}

int xrdp_painter_line(xrdpPainter* self, xrdpBitmap *dst, int x1, int y1, int x2, int y2)
{
	xrdpRect clip_rect;
	xrdpRect draw_rect;
	xrdpRect rect;
	xrdpRegion *region;
	int k;
	int dx;
	int dy;
	int rop;

	if (!self)
		return 0;

	/* todo data */

	if (dst->type == WND_TYPE_BITMAP)
		return 0;

	xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
	region = xrdp_region_create(self->wm);

	if (dst->type != WND_TYPE_OFFSCREEN)
	{
		xrdp_wm_get_vis_region(self->wm, dst, MIN(x1, x2), MIN(y1, y2), g_abs(x1 - x2) + 1, g_abs(y1 - y2) + 1,
				region, self->clip_children);
	}
	else
	{
		xrdp_region_add_rect(region, &clip_rect);
	}

	x1 += dx;
	y1 += dy;
	x2 += dx;
	y2 += dy;
	k = 0;
	rop = self->rop;

	if (rop < 0x01 || rop > 0x10)
	{
		rop = (rop & 0xF) + 1;
	}

	while (xrdp_region_get_rect(region, k, &rect) == 0)
	{
		if (rect_intersect(&rect, &clip_rect, &draw_rect))
		{
			libxrdp_orders_line(self->session, 1, x1, y1, x2, y2, rop, self->bg_color, &self->pen,
					&draw_rect);
		}

		k++;
	}

	xrdp_region_delete(region);
	return 0;
}
