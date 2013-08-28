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
 * bitmap, drawable
 * this is a object that can be drawn on with a painter
 * all windows, bitmaps, even the screen are of this type
 * maybe it should be called xrdp_drawable
 */

#include "xrdp.h"

xrdpBitmap* xrdp_bitmap_create(int width, int height, int bpp, int type, xrdpWm *wm)
{
	xrdpBitmap* self = (xrdpBitmap *) NULL;
	int Bpp = 0;

	self = (xrdpBitmap *) g_malloc(sizeof(xrdpBitmap), 1);
	self->type = type;
	self->width = width;
	self->height = height;
	self->bpp = bpp;
	Bpp = 4;

	switch (bpp)
	{
		case 8:
			Bpp = 1;
			break;

		case 15:
			Bpp = 2;
			break;

		case 16:
			Bpp = 2;
			break;

		case 24:
			Bpp = 4;
			break;

		case 32:
			Bpp = 4;
			break;
	}

	if (self->type == WND_TYPE_BITMAP)
	{
		self->data = (char*) g_malloc(width * height * Bpp, 0);
	}

	self->line_size = width * Bpp;

	self->wm = wm;
	return self;
}

void xrdp_bitmap_delete(xrdpBitmap* self)
{
	if (!self)
		return;

	if (!self->do_not_free_data)
		free(self->data);

	free(self);
}
