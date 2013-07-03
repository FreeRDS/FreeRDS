/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * xrdp-ng interprocess communication protocol
 *
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xrdp-ng/xrdp.h>

#define XRDP_ORDER_HEADER_LENGTH	6

int xrdp_read_header(wStream* s, UINT32* type, UINT32* length)
{
	Stream_Write_UINT16(s, *type);
	Stream_Write_UINT32(s, *length);
	return 0;
}

int xrdp_write_header(wStream* s, UINT32 type, UINT32 length)
{
	Stream_Write_UINT16(s, type);
	Stream_Write_UINT32(s, length);
	return 0;
}

int xrdp_read_begin_update(wStream* s, XRDP_MSG_BEGIN_UPDATE* msg)
{
	return 0;
}

int xrdp_write_begin_update(wStream* s, XRDP_MSG_BEGIN_UPDATE* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_BEGIN_UPDATE, msg->length);

	return 0;
}

int xrdp_read_end_update(wStream* s, XRDP_MSG_END_UPDATE* msg)
{
	return 0;
}

int xrdp_write_end_update(wStream* s, XRDP_MSG_END_UPDATE* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_END_UPDATE, msg->length);

	return 0;
}

int xrdp_read_opaque_rect(wStream* s, XRDP_MSG_OPAQUE_RECT* msg)
{
	Stream_Read_UINT16(s, msg->nLeftRect);
	Stream_Read_UINT16(s, msg->nTopRect);
	Stream_Read_UINT16(s, msg->nWidth);
	Stream_Read_UINT16(s, msg->nHeight);

	return 0;
}

int xrdp_write_opaque_rect(wStream* s, XRDP_MSG_OPAQUE_RECT* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 8;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_OPAQUE_RECT, msg->length);

	Stream_Write_UINT16(s, msg->nLeftRect);
	Stream_Write_UINT16(s, msg->nTopRect);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);

	return 0;
}

int xrdp_read_screen_blt(wStream* s, XRDP_MSG_SCREEN_BLT* msg)
{
	Stream_Read_UINT16(s, msg->nLeftRect);
	Stream_Read_UINT16(s, msg->nTopRect);
	Stream_Read_UINT16(s, msg->nWidth);
	Stream_Read_UINT16(s, msg->nHeight);
	Stream_Read_UINT16(s, msg->nXSrc);
	Stream_Read_UINT16(s, msg->nYSrc);

	return 0;
}

int xrdp_write_screen_blt(wStream* s, XRDP_MSG_SCREEN_BLT* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 12;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_SCREEN_BLT, msg->length);

	Stream_Write_UINT16(s, msg->nLeftRect);
	Stream_Write_UINT16(s, msg->nTopRect);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);
	Stream_Write_UINT16(s, msg->nXSrc);
	Stream_Write_UINT16(s, msg->nYSrc);

	return 0;
}

int xrdp_read_paint_rect(wStream* s, XRDP_MSG_PAINT_RECT* msg)
{
	Stream_Read_UINT16(s, msg->nLeftRect);
	Stream_Read_UINT16(s, msg->nTopRect);
	Stream_Read_UINT16(s, msg->nWidth);
	Stream_Read_UINT16(s, msg->nHeight);
	Stream_Read_UINT32(s, msg->bitmapDataLength);

	if (msg->bitmapDataLength)
	{
		Stream_GetPointer(s, msg->bitmapData);
		Stream_Seek(s, msg->bitmapDataLength);
	}
	else
	{
		Stream_Read_UINT32(s, msg->fbSegmentId);
	}

	Stream_Read_UINT16(s, msg->nWidth);
	Stream_Read_UINT16(s, msg->nHeight);
	Stream_Read_UINT16(s, msg->nXSrc);
	Stream_Read_UINT16(s, msg->nYSrc);

	return 0;
}

int xrdp_write_paint_rect(wStream* s, XRDP_MSG_PAINT_RECT* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 20;

	if (msg->fbSegmentId)
		msg->length += 4;
	else
		msg->length += msg->bitmapDataLength;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_PAINT_RECT, msg->length);

	Stream_Write_UINT16(s, msg->nLeftRect);
	Stream_Write_UINT16(s, msg->nTopRect);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);

	if (msg->fbSegmentId)
	{
		Stream_Write_UINT32(s, 0);
		Stream_Write_UINT32(s, msg->fbSegmentId);
	}
	else
	{
		Stream_Write_UINT32(s, msg->bitmapDataLength);
		Stream_Write(s, msg->bitmapData, msg->bitmapDataLength);
	}

	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);
	Stream_Write_UINT16(s, msg->nXSrc);
	Stream_Write_UINT16(s, msg->nYSrc);

	return 0;
}

int xrdp_read_set_clip(wStream* s, XRDP_MSG_SET_CLIP* msg)
{
	Stream_Read_UINT16(s, msg->x);
	Stream_Read_UINT16(s, msg->y);
	Stream_Read_UINT16(s, msg->width);
	Stream_Read_UINT16(s, msg->height);

	return 0;
}

int xrdp_write_set_clip(wStream* s, XRDP_MSG_SET_CLIP* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 8;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_SET_CLIP, msg->length);
	Stream_Write_UINT16(s, msg->x);
	Stream_Write_UINT16(s, msg->y);
	Stream_Write_UINT16(s, msg->width);
	Stream_Write_UINT16(s, msg->height);

	return 0;
}

int xrdp_read_reset_clip(wStream* s, XRDP_MSG_RESET_CLIP* msg)
{
	return 0;
}

int xrdp_write_reset_clip(wStream* s, XRDP_MSG_RESET_CLIP* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_RESET_CLIP, msg->length);

	return 0;
}

int xrdp_read_set_forecolor(wStream* s, XRDP_MSG_SET_FORECOLOR* msg)
{
	Stream_Read_UINT32(s, msg->ForeColor);

	return 0;
}

int xrdp_write_set_forecolor(wStream* s, XRDP_MSG_SET_FORECOLOR* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 4;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_SET_FORECOLOR, msg->length);
	Stream_Write_UINT32(s, msg->ForeColor);

	return 0;
}

int xrdp_read_set_backcolor(wStream* s, XRDP_MSG_SET_BACKCOLOR* msg)
{
	Stream_Read_UINT32(s, msg->BackColor);

	return 0;
}

int xrdp_write_set_backcolor(wStream* s, XRDP_MSG_SET_BACKCOLOR* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 4;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_SET_BACKCOLOR, msg->length);
	Stream_Write_UINT32(s, msg->BackColor);

	return 0;
}

int xrdp_read_set_rop2(wStream* s, XRDP_MSG_SET_ROP2* msg)
{
	Stream_Read_UINT16(s, msg->bRop2);

	return 0;
}

int xrdp_write_set_rop2(wStream* s, XRDP_MSG_SET_ROP2* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 2;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_SET_ROP2, msg->length);
	Stream_Write_UINT16(s, msg->bRop2);

	return 0;
}

int xrdp_read_set_pen(wStream* s, XRDP_MSG_SET_PEN* msg)
{
	Stream_Read_UINT16(s, msg->PenStyle);
	Stream_Read_UINT16(s, msg->PenWidth);

	return 0;
}

int xrdp_write_set_pen(wStream* s, XRDP_MSG_SET_PEN* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 4;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_SET_PEN, msg->length);

	Stream_Write_UINT16(s, msg->PenStyle);
	Stream_Write_UINT16(s, msg->PenWidth);

	return 0;
}

int xrdp_read_line_to(wStream* s, XRDP_MSG_LINE_TO* msg)
{
	Stream_Read_UINT16(s, msg->nXStart);
	Stream_Read_UINT16(s, msg->nYStart);
	Stream_Read_UINT16(s, msg->nXEnd);
	Stream_Read_UINT16(s, msg->nYEnd);

	return 0;
}

int xrdp_write_line_to(wStream* s, XRDP_MSG_LINE_TO* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 8;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_LINE_TO, msg->length);

	Stream_Write_UINT16(s, msg->nXStart);
	Stream_Write_UINT16(s, msg->nYStart);
	Stream_Write_UINT16(s, msg->nXEnd);
	Stream_Write_UINT16(s, msg->nYEnd);

	return 0;
}

int xrdp_read_set_pointer(wStream* s, XRDP_MSG_SET_POINTER* msg)
{
	Stream_Read_UINT16(s, msg->xPos);
	Stream_Read_UINT16(s, msg->yPos);

	msg->lengthXorMask = 32 * (32 * 3);
	Stream_GetPointer(s, msg->xorMaskData);
	Stream_Seek(s, msg->lengthXorMask);

	msg->lengthXorMask = 32 * (32 / 8);
	Stream_GetPointer(s, msg->andMaskData);
	Stream_Seek(s, msg->lengthAndMask);

	return 0;
}

int xrdp_write_set_pointer(wStream* s, XRDP_MSG_SET_POINTER* msg)
{
	int size = 4 + 32 * (32 * 3) + 32 * (32 / 8);
	msg->length = XRDP_ORDER_HEADER_LENGTH + size;

	if (!s)
		return msg->length;

	if (msg->xPos < 0)
		msg->xPos = 0;

	if (msg->xPos > 31)
		msg->xPos = 31;

	if (msg->yPos < 0)
		msg->yPos = 0;

	if (msg->yPos > 31)
		msg->yPos = 31;

	xrdp_write_header(s, XRDP_SERVER_SET_POINTER, msg->length);
	Stream_Write_UINT16(s, msg->xPos);
	Stream_Write_UINT16(s, msg->yPos);
	Stream_Write(s, msg->xorMaskData, 32 * (32 * 3));
	Stream_Write(s, msg->andMaskData, 32 * (32 / 8));

	return 0;
}

int xrdp_read_set_pointer_ex(wStream* s, XRDP_MSG_SET_POINTER_EX* msg)
{
	int BytesPerPixel;

	Stream_Read_UINT16(s, msg->xPos);
	Stream_Read_UINT16(s, msg->yPos);
	Stream_Read_UINT16(s, msg->xorBpp);

	BytesPerPixel = (msg->xorBpp == 0) ? 3 : (msg->xorBpp + 7) / 8;

	msg->lengthXorMask = 32 * (32 * BytesPerPixel);
	Stream_GetPointer(s, msg->xorMaskData);
	Stream_Seek(s, msg->lengthXorMask);

	msg->lengthXorMask = 32 * (32 / 8);
	Stream_GetPointer(s, msg->andMaskData);
	Stream_Seek(s, msg->lengthAndMask);

	return 0;
}

int xrdp_write_set_pointer_ex(wStream* s, XRDP_MSG_SET_POINTER_EX* msg)
{
	int BytesPerPixel = (msg->xorBpp == 0) ? 3 : (msg->xorBpp + 7) / 8;
	int size = 6 + 32 * (32 * BytesPerPixel) + 32 * (32 / 8);
	msg->length = XRDP_ORDER_HEADER_LENGTH + size;

	if (!s)
		return msg->length;

	if (msg->xPos < 0)
		msg->xPos = 0;

	if (msg->xPos > 31)
		msg->xPos = 31;

	if (msg->yPos < 0)
		msg->yPos = 0;

	if (msg->yPos > 31)
		msg->yPos = 31;

	xrdp_write_header(s, XRDP_SERVER_SET_POINTER_EX, msg->length);
	Stream_Write_UINT16(s, msg->xPos);
	Stream_Write_UINT16(s, msg->yPos);
	Stream_Write_UINT16(s, msg->xorBpp);
	Stream_Write(s, msg->xorMaskData, 32 * (32 * BytesPerPixel));
	Stream_Write(s, msg->andMaskData, 32 * (32 / 8));

	return 0;
}

int xrdp_read_create_os_surface(wStream* s, XRDP_MSG_CREATE_OS_SURFACE* msg)
{
	Stream_Read_UINT32(s, msg->index);
	Stream_Read_UINT16(s, msg->width);
	Stream_Read_UINT16(s, msg->height);

	return 0;
}

int xrdp_write_create_os_surface(wStream* s, XRDP_MSG_CREATE_OS_SURFACE* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 8;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_CREATE_OS_SURFACE, msg->length);
	Stream_Write_UINT32(s, msg->index);
	Stream_Write_UINT16(s, msg->width);
	Stream_Write_UINT16(s, msg->height);

	return 0;
}

int xrdp_read_switch_os_surface(wStream* s, XRDP_MSG_SWITCH_OS_SURFACE* msg)
{
	Stream_Read_UINT32(s, msg->index);

	return 0;
}

int xrdp_write_switch_os_surface(wStream* s, XRDP_MSG_SWITCH_OS_SURFACE* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 4;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_SWITCH_OS_SURFACE, msg->length);
	Stream_Write_UINT32(s, msg->index);

	return 0;
}

int xrdp_read_delete_os_surface(wStream* s, XRDP_MSG_DELETE_OS_SURFACE* msg)
{
	Stream_Read_UINT32(s, msg->index);

	return 0;
}

int xrdp_write_delete_os_surface(wStream* s, XRDP_MSG_DELETE_OS_SURFACE* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 4;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_DELETE_OS_SURFACE, msg->length);
	Stream_Write_UINT32(s, msg->index);

	return 0;
}

int xrdp_read_memblt(wStream* s, XRDP_MSG_MEMBLT* msg)
{
	Stream_Read_UINT16(s, msg->nLeftRect);
	Stream_Read_UINT16(s, msg->nTopRect);
	Stream_Read_UINT16(s, msg->nWidth);
	Stream_Read_UINT16(s, msg->nHeight);
	Stream_Read_UINT32(s, msg->index);
	Stream_Read_UINT16(s, msg->nXSrc);
	Stream_Read_UINT16(s, msg->nYSrc);

	return 0;
}

int xrdp_write_memblt(wStream* s, XRDP_MSG_MEMBLT* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 16;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_MEMBLT, msg->length);

	Stream_Write_UINT16(s, msg->nLeftRect);
	Stream_Write_UINT16(s, msg->nTopRect);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);
	Stream_Write_UINT32(s, msg->index);
	Stream_Write_UINT16(s, msg->nXSrc);
	Stream_Write_UINT16(s, msg->nYSrc);

	return 0;
}

int xrdp_read_set_hints(wStream* s, XRDP_MSG_SET_HINTS* msg)
{
	Stream_Read_UINT32(s, msg->hints);
	Stream_Read_UINT32(s, msg->mask);

	return 0;
}

int xrdp_write_set_hints(wStream* s, XRDP_MSG_SET_HINTS* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 8;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_SET_HINTS, msg->length);

	Stream_Write_UINT32(s, msg->hints);
	Stream_Write_UINT32(s, msg->mask);

	return 0;
}

int xrdp_read_window_new_update(wStream* s, XRDP_MSG_WINDOW_NEW_UPDATE* msg)
{
	int index;
	UINT32 flags;

	flags = WINDOW_ORDER_TYPE_WINDOW | WINDOW_ORDER_STATE_NEW;
	flags |= WINDOW_ORDER_FIELD_OWNER;
	flags |= WINDOW_ORDER_FIELD_STYLE;
	flags |= WINDOW_ORDER_FIELD_SHOW;
	flags |= WINDOW_ORDER_FIELD_TITLE;
	flags |= WINDOW_ORDER_FIELD_CLIENT_AREA_OFFSET;
	flags |= WINDOW_ORDER_FIELD_CLIENT_AREA_SIZE;
	flags |= WINDOW_ORDER_FIELD_ROOT_PARENT;
	flags |= WINDOW_ORDER_FIELD_WND_RECTS;
	flags |= WINDOW_ORDER_FIELD_VIS_OFFSET;
	flags |= WINDOW_ORDER_FIELD_VISIBILITY;

	if (flags & WINDOW_ORDER_FIELD_OWNER)
	{
		Stream_Read_UINT32(s, msg->windowId); /* windowId */
		Stream_Read_UINT32(s, msg->ownerWindowId); /* ownerWindowId */
	}

	if (flags & WINDOW_ORDER_FIELD_STYLE)
	{
		Stream_Read_UINT32(s, msg->style); /* style */
		Stream_Read_UINT32(s, msg->extendedStyle); /* extendedStyle */
	}

	if (flags & WINDOW_ORDER_FIELD_SHOW)
		Stream_Read_UINT32(s, msg->showState); /* showState */

	if (flags & WINDOW_ORDER_FIELD_TITLE)
	{
		Stream_Read_UINT16(s, msg->titleInfo.length); /* titleInfo */
		Stream_Write(s, msg->titleInfo.string, msg->titleInfo.length);
	}

	if (flags & WINDOW_ORDER_FIELD_CLIENT_AREA_OFFSET)
	{
		Stream_Read_UINT32(s, msg->clientOffsetX); /* clientOffsetX */
		Stream_Read_UINT32(s, msg->clientOffsetY); /* clientOffsetY */
	}

	if (flags & WINDOW_ORDER_FIELD_CLIENT_AREA_SIZE)
	{
		Stream_Read_UINT32(s, msg->clientAreaWidth); /* clientAreaWidth */
		Stream_Read_UINT32(s, msg->clientAreaHeight); /* clientAreaHeight */
	}

	if (flags & WINDOW_ORDER_FIELD_ROOT_PARENT)
	{
		Stream_Read_UINT32(s, msg->RPContent); /* RPContent */
		Stream_Read_UINT32(s, msg->rootParentHandle); /* rootParentHandle */
	}

	if (flags & WINDOW_ORDER_FIELD_WND_OFFSET)
	{
		Stream_Read_UINT32(s, msg->windowOffsetX); /* windowOffsetX */
		Stream_Read_UINT32(s, msg->windowOffsetY); /* windowOffsetY */
	}

	if (flags & WINDOW_ORDER_FIELD_WND_CLIENT_DELTA)
	{
		Stream_Read_UINT32(s, msg->windowClientDeltaX); /* windowClientDeltaX */
		Stream_Read_UINT32(s, msg->windowClientDeltaY); /* windowClientDeltaY */
	}

	if (flags & WINDOW_ORDER_FIELD_WND_SIZE)
	{
		Stream_Read_UINT32(s, msg->windowWidth); /* windowWidth */
		Stream_Read_UINT32(s, msg->windowHeight); /* windowHeight */
	}

	if (flags & WINDOW_ORDER_FIELD_WND_RECTS)
	{
		Stream_Read_UINT16(s, msg->numWindowRects); /* num_window_rects */

		msg->windowRects = (RECTANGLE_16*) Stream_Pointer(s);

		for (index = 0; index < msg->numWindowRects; index++)
		{
			Stream_Read_UINT16(s, msg->windowRects[index].left); /* left */
			Stream_Read_UINT16(s, msg->windowRects[index].top); /* top */
			Stream_Read_UINT16(s, msg->windowRects[index].right); /* right */
			Stream_Read_UINT16(s, msg->windowRects[index].bottom); /* bottom */
		}
	}

	if (flags & WINDOW_ORDER_FIELD_VIS_OFFSET)
	{
		Stream_Read_UINT32(s, msg->visibleOffsetX); /* visibleOffsetX */
		Stream_Read_UINT32(s, msg->visibleOffsetY); /* visibleOffsetY */
	}

	if (flags & WINDOW_ORDER_FIELD_VISIBILITY)
	{
		Stream_Read_UINT16(s, msg->numVisibilityRects); /* numVisibilityRects */

		for (index = 0; index < msg->numVisibilityRects; index++)
		{
			Stream_Read_UINT16(s, msg->visibilityRects[index].left); /* left */
			Stream_Read_UINT16(s, msg->visibilityRects[index].top); /* top */
			Stream_Read_UINT16(s, msg->visibilityRects[index].right); /* right */
			Stream_Read_UINT16(s, msg->visibilityRects[index].bottom); /* bottom */
		}
	}

	Stream_Read_UINT32(s, flags); /* flags */

	return 0;
}

int xrdp_write_window_new_update(wStream* s, XRDP_MSG_WINDOW_NEW_UPDATE* msg)
{
	int index;
	UINT32 flags;
	msg->length = XRDP_ORDER_HEADER_LENGTH +
			(5 * 4) + (2 + msg->titleInfo.length) + (12 * 4) +
			(2 + msg->numWindowRects * 8) + (4 + 4) +
			(2 + msg->numVisibilityRects * 8) + 4;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_WINDOW_NEW_UPDATE, msg->length);

	flags = WINDOW_ORDER_TYPE_WINDOW | WINDOW_ORDER_STATE_NEW;

	Stream_Write_UINT32(s, msg->windowId); /* windowId */
	Stream_Write_UINT32(s, msg->ownerWindowId); /* ownerWindowId */
	flags |= WINDOW_ORDER_FIELD_OWNER;

	Stream_Write_UINT32(s, msg->style); /* style */
	Stream_Write_UINT32(s, msg->extendedStyle); /* extendedStyle */
	flags |= WINDOW_ORDER_FIELD_STYLE;

	Stream_Write_UINT32(s, msg->showState); /* showState */
	flags |= WINDOW_ORDER_FIELD_SHOW;

	Stream_Write_UINT16(s, msg->titleInfo.length); /* titleInfo */
	Stream_Write(s, msg->titleInfo.string, msg->titleInfo.length);
	flags |= WINDOW_ORDER_FIELD_TITLE;

	Stream_Write_UINT32(s, msg->clientOffsetX); /* clientOffsetX */
	Stream_Write_UINT32(s, msg->clientOffsetY); /* clientOffsetY */
	flags |= WINDOW_ORDER_FIELD_CLIENT_AREA_OFFSET;

	Stream_Write_UINT32(s, msg->clientAreaWidth); /* clientAreaWidth */
	Stream_Write_UINT32(s, msg->clientAreaHeight); /* clientAreaHeight */
	flags |= WINDOW_ORDER_FIELD_CLIENT_AREA_SIZE;

	Stream_Write_UINT32(s, msg->RPContent); /* RPContent */
	Stream_Write_UINT32(s, msg->rootParentHandle); /* rootParentHandle */
	flags |= WINDOW_ORDER_FIELD_ROOT_PARENT;

	Stream_Write_UINT32(s, msg->windowOffsetX); /* windowOffsetX */
	Stream_Write_UINT32(s, msg->windowOffsetY); /* windowOffsetY */
	flags |= WINDOW_ORDER_FIELD_WND_OFFSET;

	Stream_Write_UINT32(s, msg->windowClientDeltaX); /* windowClientDeltaX */
	Stream_Write_UINT32(s, msg->windowClientDeltaY); /* windowClientDeltaY */
	flags |= WINDOW_ORDER_FIELD_WND_CLIENT_DELTA;

	Stream_Write_UINT32(s, msg->windowWidth); /* windowWidth */
	Stream_Write_UINT32(s, msg->windowHeight); /* windowHeight */
	flags |= WINDOW_ORDER_FIELD_WND_SIZE;

	Stream_Write_UINT16(s, msg->numWindowRects); /* num_window_rects */

	for (index = 0; index < msg->numWindowRects; index++)
	{
		Stream_Write_UINT16(s, msg->windowRects[index].left); /* left */
		Stream_Write_UINT16(s, msg->windowRects[index].top); /* top */
		Stream_Write_UINT16(s, msg->windowRects[index].right); /* right */
		Stream_Write_UINT16(s, msg->windowRects[index].bottom); /* bottom */
	}

	flags |= WINDOW_ORDER_FIELD_WND_RECTS;

	Stream_Write_UINT32(s, msg->visibleOffsetX); /* visibleOffsetX */
	Stream_Write_UINT32(s, msg->visibleOffsetY); /* visibleOffsetY */
	flags |= WINDOW_ORDER_FIELD_VIS_OFFSET;

	Stream_Write_UINT16(s, msg->numVisibilityRects); /* numVisibilityRects */

	for (index = 0; index < msg->numVisibilityRects; index++)
	{
		Stream_Write_UINT16(s, msg->visibilityRects[index].left); /* left */
		Stream_Write_UINT16(s, msg->visibilityRects[index].top); /* top */
		Stream_Write_UINT16(s, msg->visibilityRects[index].right); /* right */
		Stream_Write_UINT16(s, msg->visibilityRects[index].bottom); /* bottom */
	}

	flags |= WINDOW_ORDER_FIELD_VISIBILITY;

	Stream_Write_UINT32(s, flags); /* flags */

	return 0;
}

int xrdp_read_window_delete(wStream* s, XRDP_MSG_WINDOW_DELETE* msg)
{
	Stream_Read_UINT32(s, msg->windowId);

	return 0;
}

int xrdp_write_window_delete(wStream* s, XRDP_MSG_WINDOW_DELETE* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 4;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_WINDOW_DELETE, msg->length);

	Stream_Write_UINT32(s, msg->windowId);

	return 0;
}

int xrdp_read_shared_framebuffer(wStream* s, XRDP_MSG_SHARED_FRAMEBUFFER* msg)
{
	Stream_Read_UINT32(s, msg->width);
	Stream_Read_UINT32(s, msg->height);
	Stream_Read_UINT32(s, msg->scanline);
	Stream_Read_UINT32(s, msg->segmentId);
	Stream_Read_UINT32(s, msg->bitsPerPixel);
	Stream_Read_UINT32(s, msg->bytesPerPixel);

	return 0;
}

int xrdp_write_shared_framebuffer(wStream* s, XRDP_MSG_SHARED_FRAMEBUFFER* msg)
{
	msg->length = XRDP_ORDER_HEADER_LENGTH + 24;

	if (!s)
		return msg->length;

	xrdp_write_header(s, XRDP_SERVER_SHARED_FRAMEBUFFER, msg->length);

	Stream_Write_UINT32(s, msg->width);
	Stream_Write_UINT32(s, msg->height);
	Stream_Write_UINT32(s, msg->scanline);
	Stream_Write_UINT32(s, msg->segmentId);
	Stream_Write_UINT32(s, msg->bitsPerPixel);
	Stream_Write_UINT32(s, msg->bytesPerPixel);

	return 0;
}

int xrdp_prepare_msg(wStream* s, XRDP_MSG_COMMON* msg)
{
	switch (msg->type)
	{
		case XRDP_SERVER_BEGIN_UPDATE:
			xrdp_write_begin_update(s, (XRDP_MSG_BEGIN_UPDATE*) msg);
			break;

		case XRDP_SERVER_END_UPDATE:
			xrdp_write_end_update(s, (XRDP_MSG_END_UPDATE*) msg);
			break;

		case XRDP_SERVER_OPAQUE_RECT:
			xrdp_write_opaque_rect(s, (XRDP_MSG_OPAQUE_RECT*) msg);
			break;

		case XRDP_SERVER_SCREEN_BLT:
			xrdp_write_screen_blt(s, (XRDP_MSG_SCREEN_BLT*) msg);
			break;

		case XRDP_SERVER_PAINT_RECT:
			xrdp_write_paint_rect(s, (XRDP_MSG_PAINT_RECT*) msg);
			break;

		case XRDP_SERVER_SET_CLIP:
			xrdp_write_set_clip(s, (XRDP_MSG_SET_CLIP*) msg);
			break;

		case XRDP_SERVER_RESET_CLIP:
			xrdp_write_reset_clip(s, (XRDP_MSG_RESET_CLIP*) msg);
			break;

		case XRDP_SERVER_SET_FORECOLOR:
			xrdp_write_set_forecolor(s, (XRDP_MSG_SET_FORECOLOR*) msg);
			break;

		case XRDP_SERVER_SET_BACKCOLOR:
			xrdp_write_set_backcolor(s, (XRDP_MSG_SET_BACKCOLOR*) msg);
			break;

		case XRDP_SERVER_SET_ROP2:
			xrdp_write_set_rop2(s, (XRDP_MSG_SET_ROP2*) msg);
			break;

		case XRDP_SERVER_SET_PEN:
			xrdp_write_set_pen(s, (XRDP_MSG_SET_PEN*) msg);
			break;

		case XRDP_SERVER_LINE_TO:
			xrdp_write_line_to(s, (XRDP_MSG_LINE_TO*) msg);
			break;

		case XRDP_SERVER_SET_POINTER:
			xrdp_write_set_pointer(s, (XRDP_MSG_SET_POINTER*) msg);
			break;

		case XRDP_SERVER_SET_POINTER_EX:
			xrdp_write_set_pointer_ex(s, (XRDP_MSG_SET_POINTER_EX*) msg);
			break;

		case XRDP_SERVER_CREATE_OS_SURFACE:
			xrdp_write_create_os_surface(s, (XRDP_MSG_CREATE_OS_SURFACE*) msg);
			break;

		case XRDP_SERVER_SWITCH_OS_SURFACE:
			xrdp_write_switch_os_surface(s, (XRDP_MSG_SWITCH_OS_SURFACE*) msg);
			break;

		case XRDP_SERVER_DELETE_OS_SURFACE:
			xrdp_write_delete_os_surface(s, (XRDP_MSG_DELETE_OS_SURFACE*) msg);
			break;

		case XRDP_SERVER_MEMBLT:
			xrdp_write_memblt(s, (XRDP_MSG_MEMBLT*) msg);
			break;

		case XRDP_SERVER_SET_HINTS:
			xrdp_write_set_hints(s, (XRDP_MSG_SET_HINTS*) msg);
			break;

		case XRDP_SERVER_WINDOW_NEW_UPDATE:
			xrdp_write_window_new_update(s, (XRDP_MSG_WINDOW_NEW_UPDATE*) msg);
			break;

		case XRDP_SERVER_WINDOW_DELETE:
			xrdp_write_window_delete(s, (XRDP_MSG_WINDOW_DELETE*) msg);
			break;

		case XRDP_SERVER_SHARED_FRAMEBUFFER:
			xrdp_write_shared_framebuffer(s, (XRDP_MSG_SHARED_FRAMEBUFFER*) msg);
			break;

		default:
			msg->length = XRDP_ORDER_HEADER_LENGTH;
			break;
	}

	return msg->length;
}

char* xrdp_get_msg_type_string(UINT32 type)
{
	switch (type)
	{
		case XRDP_SERVER_BEGIN_UPDATE:
			return "BeginUpdate";
			break;

		case XRDP_SERVER_END_UPDATE:
			return "EndUpdate";
			break;

		case XRDP_SERVER_OPAQUE_RECT:
			return "OpaqueRect";
			break;

		case XRDP_SERVER_SCREEN_BLT:
			return "ScreenBlt";
			break;

		case XRDP_SERVER_PAINT_RECT:
			return "PaintRect";
			break;

		case XRDP_SERVER_SET_CLIP:
			return "SetClip";
			break;

		case XRDP_SERVER_RESET_CLIP:
			return "ResetClip";
			break;

		case XRDP_SERVER_SET_FORECOLOR:
			return "SetForeColor";
			break;

		case XRDP_SERVER_SET_BACKCOLOR:
			return "SetBackColor";
			break;

		case XRDP_SERVER_SET_ROP2:
			return "SetRop2";
			break;

		case XRDP_SERVER_SET_PEN:
			return "SetPen";
			break;

		case XRDP_SERVER_LINE_TO:
			return "LineTo";
			break;

		case XRDP_SERVER_SET_POINTER:
			return "SetPointer";
			break;

		case XRDP_SERVER_SET_POINTER_EX:
			return "SetPointerEx";
			break;

		case XRDP_SERVER_CREATE_OS_SURFACE:
			return "CreateOffscreenSurface";
			break;

		case XRDP_SERVER_SWITCH_OS_SURFACE:
			return "SwitchOffscreenSurface";
			break;

		case XRDP_SERVER_DELETE_OS_SURFACE:
			return "DeleteOffscreenSurface";
			break;

		case XRDP_SERVER_MEMBLT:
			return "MemBlt";
			break;

		case XRDP_SERVER_SET_HINTS:
			return "SetHints";
			break;

		case XRDP_SERVER_WINDOW_NEW_UPDATE:
			return "WindowNewUpdate";
			break;

		case XRDP_SERVER_WINDOW_DELETE:
			return "WindowDelete";
			break;

		case XRDP_SERVER_SHARED_FRAMEBUFFER:
			return "SharedFrameBuffer";
			break;

		default:
			return "Unknown";
			break;
	}

	return "Unknown";
}
