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
 * region
 */

#include "xrdp.h"

/*****************************************************************************/
xrdpRegion *
xrdp_region_create(xrdpWm *wm)
{
	xrdpRegion *self;

	self = (xrdpRegion *) g_malloc(sizeof(xrdpRegion), 1);
	self->wm = wm;
	self->rects = list_create();
	self->rects->auto_free = 1;
	return self;
}

/*****************************************************************************/
void 
xrdp_region_delete(xrdpRegion *self)
{
	if (self == 0)
	{
		return;
	}

	list_delete(self->rects);
	g_free(self);
}

/*****************************************************************************/
int 
xrdp_region_add_rect(xrdpRegion *self, xrdpRect *rect)
{
	xrdpRect *r;

	r = (xrdpRect *) g_malloc(sizeof(xrdpRect), 1);
	*r = *rect;
	list_add_item(self->rects, (long) r);
	return 0;
}

/*****************************************************************************/
int 
xrdp_region_insert_rect(xrdpRegion *self, int i, int left, int top, int right, int bottom)
{
	xrdpRect *r;

	r = (xrdpRect *) g_malloc(sizeof(xrdpRect), 1);
	r->left = left;
	r->top = top;
	r->right = right;
	r->bottom = bottom;
	list_insert_item(self->rects, i, (long) r);
	return 0;
}

/*****************************************************************************/
int 
xrdp_region_subtract_rect(xrdpRegion *self,
                          xrdpRect *rect)
{
    xrdpRect *r;
    xrdpRect rect1;
    int i;

    for (i = self->rects->count - 1; i >= 0; i--)
    {
        r = (xrdpRect *)list_get_item(self->rects, i);
        rect1 = *r;
        r = &rect1;

        if (rect->left <= r->left &&
                rect->top <= r->top &&
                rect->right >= r->right &&
                rect->bottom >= r->bottom)
        {
            /* rect is not visible */
            list_remove_item(self->rects, i);
        }
        else if (rect->right < r->left ||
                 rect->bottom < r->top ||
                 rect->top > r->bottom ||
                 rect->left > r->right)
        {
            /* rect are not related */
        }
        else if (rect->left <= r->left &&
                 rect->right >= r->right &&
                 rect->bottom < r->bottom &&
                 rect->top <= r->top)
        {
            /* partially covered(whole top) */
            list_remove_item(self->rects, i);
            xrdp_region_insert_rect(self, i, r->left, rect->bottom,
                                    r->right, r->bottom);
        }
        else if (rect->top <= r->top &&
                 rect->bottom >= r->bottom &&
                 rect->right < r->right &&
                 rect->left <= r->left)
        {
            /* partially covered(left) */
            list_remove_item(self->rects, i);
            xrdp_region_insert_rect(self, i, rect->right, r->top,
                                    r->right, r->bottom);
        }
        else if (rect->left <= r->left &&
                 rect->right >= r->right &&
                 rect->top > r->top &&
                 rect->bottom >= r->bottom)
        {
            /* partially covered(bottom) */
            list_remove_item(self->rects, i);
            xrdp_region_insert_rect(self, i, r->left, r->top,
                                    r->right, rect->top);
        }
        else if (rect->top <= r->top &&
                 rect->bottom >= r->bottom &&
                 rect->left > r->left &&
                 rect->right >= r->right)
        {
            /* partially covered(right) */
            list_remove_item(self->rects, i);
            xrdp_region_insert_rect(self, i, r->left, r->top,
                                    rect->left, r->bottom);
        }
        else if (rect->left <= r->left &&
                 rect->top <= r->top &&
                 rect->right < r->right &&
                 rect->bottom < r->bottom)
        {
            /* partially covered(top left) */
            list_remove_item(self->rects, i);
            xrdp_region_insert_rect(self, i, rect->right, r->top,
                                    r->right, rect->bottom);
            xrdp_region_insert_rect(self, i, r->left, rect->bottom,
                                    r->right, r->bottom);
        }
        else if (rect->left <= r->left &&
                 rect->bottom >= r->bottom &&
                 rect->right < r->right &&
                 rect->top > r->top)
        {
            /* partially covered(bottom left) */
            list_remove_item(self->rects, i);
            xrdp_region_insert_rect(self, i, r->left, r->top,
                                    r->right, rect->top);
            xrdp_region_insert_rect(self, i, rect->right, rect->top,
                                    r->right, r->bottom);
        }
        else if (rect->left > r->left &&
                 rect->right >= r->right &&
                 rect->top <= r->top &&
                 rect->bottom < r->bottom)
        {
            /* partially covered(top right) */
            list_remove_item(self->rects, i);
            xrdp_region_insert_rect(self, i, r->left, r->top,
                                    rect->left, r->bottom);
            xrdp_region_insert_rect(self, i, rect->left, rect->bottom,
                                    r->right, r->bottom);
        }
        else if (rect->left > r->left &&
                 rect->right >= r->right &&
                 rect->top > r->top &&
                 rect->bottom >= r->bottom)
        {
            /* partially covered(bottom right) */
            list_remove_item(self->rects, i);
            xrdp_region_insert_rect(self, i, r->left, r->top,
                                    r->right, rect->top);
            xrdp_region_insert_rect(self, i, r->left, rect->top,
                                    rect->left, r->bottom);
        }
        else if (rect->left > r->left &&
                 rect->top <= r->top &&
                 rect->right < r->right &&
                 rect->bottom >= r->bottom)
        {
            /* 2 rects, one on each end */
            list_remove_item(self->rects, i);
            xrdp_region_insert_rect(self, i, r->left, r->top,
                                    rect->left, r->bottom);
            xrdp_region_insert_rect(self, i, rect->right, r->top,
                                    r->right, r->bottom);
        }
        else if (rect->left <= r->left &&
                 rect->top > r->top &&
                 rect->right >= r->right &&
                 rect->bottom < r->bottom)
        {
            /* 2 rects, one on each end */
            list_remove_item(self->rects, i);
            xrdp_region_insert_rect(self, i, r->left, r->top,
                                    r->right, rect->top);
            xrdp_region_insert_rect(self, i, r->left, rect->bottom,
                                    r->right, r->bottom);
        }
        else if (rect->left > r->left &&
                 rect->right < r->right &&
                 rect->top <= r->top &&
                 rect->bottom < r->bottom)
        {
            /* partially covered(top) */
            list_remove_item(self->rects, i);
            xrdp_region_insert_rect(self, i, r->left, r->top,
                                    rect->left, r->bottom);
            xrdp_region_insert_rect(self, i, rect->left, rect->bottom,
                                    rect->right, r->bottom);
            xrdp_region_insert_rect(self, i, rect->right, r->top,
                                    r->right, r->bottom);
        }
        else if (rect->top > r->top &&
                 rect->bottom < r->bottom &&
                 rect->left <= r->left &&
                 rect->right < r->right)
        {
            /* partially covered(left) */
            list_remove_item(self->rects, i);
            xrdp_region_insert_rect(self, i, r->left, r->top,
                                    r->right, rect->top);
            xrdp_region_insert_rect(self, i, rect->right, rect->top,
                                    r->right, rect->bottom);
            xrdp_region_insert_rect(self, i, r->left, rect->bottom,
                                    r->right, r->bottom);
        }
        else if (rect->left > r->left &&
                 rect->right < r->right &&
                 rect->bottom >= r->bottom &&
                 rect->top > r->top)
        {
            /* partially covered(bottom) */
            list_remove_item(self->rects, i);
            xrdp_region_insert_rect(self, i, r->left, r->top,
                                    rect->left, r->bottom);
            xrdp_region_insert_rect(self, i, rect->left, r->top,
                                    rect->right, rect->top);
            xrdp_region_insert_rect(self, i, rect->right, r->top,
                                    r->right, r->bottom);
        }
        else if (rect->top > r->top &&
                 rect->bottom < r->bottom &&
                 rect->right >= r->right &&
                 rect->left > r->left)
        {
            /* partially covered(right) */
            list_remove_item(self->rects, i);
            xrdp_region_insert_rect(self, i, r->left, r->top,
                                    r->right, rect->top);
            xrdp_region_insert_rect(self, i, r->left, rect->top,
                                    rect->left, rect->bottom);
            xrdp_region_insert_rect(self, i, r->left, rect->bottom,
                                    r->right, r->bottom);
        }
        else if (rect->left > r->left &&
                 rect->top > r->top &&
                 rect->right < r->right &&
                 rect->bottom < r->bottom)
        {
            /* totally contained, 4 rects */
            list_remove_item(self->rects, i);
            xrdp_region_insert_rect(self, i, r->left, r->top,
                                    r->right, rect->top);
            xrdp_region_insert_rect(self, i, r->left, rect->top,
                                    rect->left, rect->bottom);
            xrdp_region_insert_rect(self, i, r->left, rect->bottom,
                                    r->right, r->bottom);
            xrdp_region_insert_rect(self, i, rect->right, rect->top,
                                    r->right, rect->bottom);
        }
        else
        {
            g_writeln("error in xrdp_region_subtract_rect");
        }
    }

    return 0;
}

/*****************************************************************************/
int 
xrdp_region_get_rect(xrdpRegion *self, int index, xrdpRect *rect)
{
	xrdpRect *r;

	r = (xrdpRect *) list_get_item(self->rects, index);

	if (r == 0)
	{
		return 1;
	}

	*rect = *r;
	return 0;
}
