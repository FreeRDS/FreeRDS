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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xrdp.h"

xrdpPainter* xrdp_painter_create(xrdpWm* wm, xrdpSession* session)
{
	xrdpPainter* self;

	self = (xrdpPainter*) g_malloc(sizeof(xrdpPainter), 1);
	self->wm = wm;
	self->session = session;
	self->clip_children = 1;

	return self;
}

void xrdp_painter_delete(xrdpPainter* self)
{
	if (!self)
		return;

	free(self);
}

int xrdp_painter_begin_update(xrdpPainter* self)
{
	if (!self)
		return 0;

	libxrdp_orders_init(self->session);

	return 0;
}

int xrdp_painter_end_update(xrdpPainter* self)
{
	if (!self)
		return 0;

	libxrdp_orders_send(self->session);

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
