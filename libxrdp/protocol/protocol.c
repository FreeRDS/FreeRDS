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

int xrdp_write_header(wStream* s, int type, int length)
{
	Stream_Write_UINT16(s, type);
	Stream_Write_UINT32(s, length);
	return 0;
}

int xrdp_write_begin_update(wStream* s, XRDP_MSG_BEGIN_UPDATE* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_BEGIN_UPDATE, length);

	return 0;
}

int xrdp_write_end_update(wStream* s, XRDP_MSG_END_UPDATE* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_END_UPDATE, length);

	return 0;
}

int xrdp_write_opaque_rect(wStream* s, XRDP_MSG_OPAQUE_RECT* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 8;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_OPAQUE_RECT, length);

	Stream_Write_UINT16(s, msg->nLeftRect);
	Stream_Write_UINT16(s, msg->nTopRect);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);

	return 0;
}

int xrdp_write_screen_blt(wStream* s, XRDP_MSG_SCREEN_BLT* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 12;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_SCREEN_BLT, length);

	Stream_Write_UINT16(s, msg->nLeftRect);
	Stream_Write_UINT16(s, msg->nTopRect);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);
	Stream_Write_UINT16(s, msg->nXSrc);
	Stream_Write_UINT16(s, msg->nYSrc);

	return 0;
}

int xrdp_write_paint_rect(wStream* s, XRDP_MSG_PAINT_RECT* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 20 + msg->bitmapDataLength;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_PAINT_RECT, length);

	Stream_Write_UINT16(s, msg->nLeftRect);
	Stream_Write_UINT16(s, msg->nTopRect);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);
	Stream_Write_UINT32(s, msg->bitmapDataLength);
	Stream_Write(s, msg->bitmapData, msg->bitmapDataLength);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);
	Stream_Write_UINT16(s, msg->nXSrc);
	Stream_Write_UINT16(s, msg->nYSrc);

	return 0;
}

int xrdp_write_set_clip(wStream* s, XRDP_MSG_SET_CLIP* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 8;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_SET_CLIP, length);
	Stream_Write_UINT16(s, msg->x);
	Stream_Write_UINT16(s, msg->y);
	Stream_Write_UINT16(s, msg->width);
	Stream_Write_UINT16(s, msg->height);

	return 0;
}

int xrdp_write_reset_clip(wStream* s, XRDP_MSG_RESET_CLIP* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_RESET_CLIP, length);

	return 0;
}

int xrdp_write_set_forecolor(wStream* s, XRDP_MSG_SET_FORECOLOR* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 4;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_SET_FORECOLOR, length);
	Stream_Write_UINT32(s, msg->ForeColor);

	return 0;
}

int xrdp_write_set_backcolor(wStream* s, XRDP_MSG_SET_BACKCOLOR* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 4;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_SET_BACKCOLOR, length);
	Stream_Write_UINT32(s, msg->BackColor);

	return 0;
}

int xrdp_write_set_rop2(wStream* s, XRDP_MSG_SET_ROP2* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 2;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_SET_ROP2, length);
	Stream_Write_UINT16(s, msg->bRop2);

	return 0;
}

int xrdp_write_set_pen(wStream* s, XRDP_MSG_SET_PEN* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 4;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_SET_PEN, length);

	Stream_Write_UINT16(s, msg->PenStyle);
	Stream_Write_UINT16(s, msg->PenWidth);

	return 0;
}

int xrdp_write_line_to(wStream* s, XRDP_MSG_LINE_TO* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 8;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_LINE_TO, length);

	Stream_Write_UINT16(s, msg->nXStart);
	Stream_Write_UINT16(s, msg->nYStart);
	Stream_Write_UINT16(s, msg->nXEnd);
	Stream_Write_UINT16(s, msg->nYEnd);

	return 0;
}

int xrdp_write_set_pointer(wStream* s, XRDP_MSG_SET_POINTER* msg)
{
	int size = 4 + 32 * (32 * 3) + 32 * (32 / 8);
	int length = XRDP_ORDER_HEADER_LENGTH + size;

	if (!s)
		return length;

	if (msg->xPos < 0)
		msg->xPos = 0;

	if (msg->xPos > 31)
		msg->xPos = 31;

	if (msg->yPos < 0)
		msg->yPos = 0;

	if (msg->yPos > 31)
		msg->yPos = 31;

	xrdp_write_header(s, XRDP_SERVER_SET_POINTER, length);
	Stream_Write_UINT16(s, msg->xPos);
	Stream_Write_UINT16(s, msg->yPos);
	Stream_Write(s, msg->xorMaskData, 32 * (32 * 3));
	Stream_Write(s, msg->andMaskData, 32 * (32 / 8));

	return 0;
}

int xrdp_write_set_pointer_ex(wStream* s, XRDP_MSG_SET_POINTER_EX* msg)
{
	int BytesPerPixel = (msg->xorBpp == 0) ? 3 : (msg->xorBpp + 7) / 8;
	int size = 6 + 32 * (32 * BytesPerPixel) + 32 * (32 / 8);
	int length = XRDP_ORDER_HEADER_LENGTH + size;

	if (!s)
		return length;

	if (msg->xPos < 0)
		msg->xPos = 0;

	if (msg->xPos > 31)
		msg->xPos = 31;

	if (msg->yPos < 0)
		msg->yPos = 0;

	if (msg->yPos > 31)
		msg->yPos = 31;

	xrdp_write_header(s, XRDP_SERVER_SET_POINTER_EX, length);
	Stream_Write_UINT16(s, msg->xPos);
	Stream_Write_UINT16(s, msg->yPos);
	Stream_Write_UINT16(s, msg->xorBpp);
	Stream_Write(s, msg->xorMaskData, 32 * (32 * BytesPerPixel));
	Stream_Write(s, msg->andMaskData, 32 * (32 / 8));

	return 0;
}

int xrdp_write_create_os_surface(wStream* s, XRDP_MSG_CREATE_OS_SURFACE* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 8;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_CREATE_OS_SURFACE, length);
	Stream_Write_UINT32(s, msg->index);
	Stream_Write_UINT16(s, msg->width);
	Stream_Write_UINT16(s, msg->height);

	return 0;
}

int xrdp_write_switch_os_surface(wStream* s, XRDP_MSG_SWITCH_OS_SURFACE* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 4;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_SWITCH_OS_SURFACE, length);
	Stream_Write_UINT32(s, msg->index);

	return 0;
}

int xrdp_write_delete_os_surface(wStream* s, XRDP_MSG_DELETE_OS_SURFACE* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 4;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_DELETE_OS_SURFACE, length);
	Stream_Write_UINT32(s, msg->index);

	return 0;
}

int xrdp_write_memblt(wStream* s, XRDP_MSG_MEMBLT* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 16;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_MEMBLT, length);

	Stream_Write_UINT16(s, msg->nLeftRect);
	Stream_Write_UINT16(s, msg->nTopRect);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);
	Stream_Write_UINT32(s, msg->index);
	Stream_Write_UINT16(s, msg->nXSrc);
	Stream_Write_UINT16(s, msg->nYSrc);

	return 0;
}

int xrdp_write_set_hints(wStream* s, XRDP_MSG_SET_HINTS* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 8;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_SET_HINTS, length);

	Stream_Write_UINT32(s, msg->hints);
	Stream_Write_UINT32(s, msg->mask);

	return 0;
}

int xrdp_write_window_new_update(wStream* s, XRDP_MSG_WINDOW_NEW_UPDATE* msg)
{
	int index;
	UINT32 flags;
	int length = XRDP_ORDER_HEADER_LENGTH +
			(5 * 4) + (2 + msg->titleInfo.length) + (12 * 4) +
			(2 + msg->numWindowRects * 8) + (4 + 4) +
			(2 + msg->numVisibilityRects * 8) + 4;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_WINDOW_NEW_UPDATE, length);

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

int xrdp_write_window_delete(wStream* s, XRDP_MSG_WINDOW_DELETE* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 4;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_WINDOW_DELETE, length);

	Stream_Write_UINT32(s, msg->windowId);

	return 0;
}

int xrdp_write_create_framebuffer(wStream* s, XRDP_MSG_CREATE_FRAMEBUFFER* msg)
{
	int length = XRDP_ORDER_HEADER_LENGTH + 24;

	if (!s)
		return length;

	xrdp_write_header(s, XRDP_SERVER_CREATE_FRAMEBUFFER, length);

	Stream_Write_UINT32(s, msg->width);
	Stream_Write_UINT32(s, msg->height);
	Stream_Write_UINT32(s, msg->scanline);
	Stream_Write_UINT32(s, msg->segmentId);
	Stream_Write_UINT32(s, msg->bitsPerPixel);
	Stream_Write_UINT32(s, msg->bytesPerPixel);

	return 0;
}

int xrdp_prepare_msg(XRDP_MSG_COMMON* msg, UINT32 type)
{
	msg->type = type;

	switch (msg->type)
	{
		case XRDP_SERVER_BEGIN_UPDATE:
			msg->length = xrdp_write_begin_update(NULL, (XRDP_MSG_BEGIN_UPDATE*) msg);
			break;

		case XRDP_SERVER_END_UPDATE:
			msg->length = xrdp_write_end_update(NULL, (XRDP_MSG_END_UPDATE*) msg);
			break;

		case XRDP_SERVER_OPAQUE_RECT:
			msg->length = xrdp_write_opaque_rect(NULL, (XRDP_MSG_OPAQUE_RECT*) msg);
			break;

		case XRDP_SERVER_SCREEN_BLT:
			msg->length = xrdp_write_screen_blt(NULL, (XRDP_MSG_SCREEN_BLT*) msg);
			break;

		case XRDP_SERVER_PAINT_RECT:
			msg->length = xrdp_write_paint_rect(NULL, (XRDP_MSG_PAINT_RECT*) msg);
			break;

		case XRDP_SERVER_SET_CLIP:
			msg->length = xrdp_write_set_clip(NULL, (XRDP_MSG_SET_CLIP*) msg);
			break;

		case XRDP_SERVER_RESET_CLIP:
			msg->length = xrdp_write_reset_clip(NULL, (XRDP_MSG_RESET_CLIP*) msg);
			break;

		case XRDP_SERVER_SET_FORECOLOR:
			msg->length = xrdp_write_set_forecolor(NULL, (XRDP_MSG_SET_FORECOLOR*) msg);
			break;

		case XRDP_SERVER_SET_BACKCOLOR:
			msg->length = xrdp_write_set_backcolor(NULL, (XRDP_MSG_SET_BACKCOLOR*) msg);
			break;

		case XRDP_SERVER_SET_ROP2:
			msg->length = xrdp_write_set_rop2(NULL, (XRDP_MSG_SET_ROP2*) msg);
			break;

		case XRDP_SERVER_SET_PEN:
			msg->length = xrdp_write_set_pen(NULL, (XRDP_MSG_SET_PEN*) msg);
			break;

		case XRDP_SERVER_LINE_TO:
			msg->length = xrdp_write_line_to(NULL, (XRDP_MSG_LINE_TO*) msg);
			break;

		case XRDP_SERVER_SET_POINTER:
			msg->length = xrdp_write_set_pointer(NULL, (XRDP_MSG_SET_POINTER*) msg);
			break;

		case XRDP_SERVER_SET_POINTER_EX:
			msg->length = xrdp_write_set_pointer_ex(NULL, (XRDP_MSG_SET_POINTER_EX*) msg);
			break;

		case XRDP_SERVER_CREATE_OS_SURFACE:
			msg->length = xrdp_write_create_os_surface(NULL, (XRDP_MSG_CREATE_OS_SURFACE*) msg);
			break;

		case XRDP_SERVER_SWITCH_OS_SURFACE:
			msg->length = xrdp_write_switch_os_surface(NULL, (XRDP_MSG_SWITCH_OS_SURFACE*) msg);
			break;

		case XRDP_SERVER_DELETE_OS_SURFACE:
			msg->length = xrdp_write_delete_os_surface(NULL, (XRDP_MSG_DELETE_OS_SURFACE*) msg);
			break;

		case XRDP_SERVER_MEMBLT:
			msg->length = xrdp_write_memblt(NULL, (XRDP_MSG_MEMBLT*) msg);
			break;

		case XRDP_SERVER_SET_HINTS:
			msg->length = xrdp_write_set_hints(NULL, (XRDP_MSG_SET_HINTS*) msg);
			break;

		case XRDP_SERVER_WINDOW_NEW_UPDATE:
			msg->length = xrdp_write_window_new_update(NULL, (XRDP_MSG_WINDOW_NEW_UPDATE*) msg);
			break;

		case XRDP_SERVER_WINDOW_DELETE:
			msg->length = xrdp_write_window_delete(NULL, (XRDP_MSG_WINDOW_DELETE*) msg);
			break;

		case XRDP_SERVER_CREATE_FRAMEBUFFER:
			msg->length = xrdp_write_create_framebuffer(NULL, (XRDP_MSG_CREATE_FRAMEBUFFER*) msg);
			break;

		default:
			msg->length = XRDP_ORDER_HEADER_LENGTH;
			break;
	}

	return msg->length;
}
