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

typedef int (*pXrdpMessageRead)(wStream* s, XRDP_MSG_COMMON* msg);
typedef int (*pXrdpMessageWrite)(wStream* s, XRDP_MSG_COMMON* msg);
typedef void* (*pXrdpMessageCopy)(XRDP_MSG_COMMON* msg);
typedef void (*pXrdpMessageFree)(XRDP_MSG_COMMON* msg);

struct _XRDP_MSG_DEFINITION
{
	int Size;
	const char* Name;
	pXrdpMessageRead Read;
	pXrdpMessageWrite Write;
	pXrdpMessageCopy Copy;
	pXrdpMessageFree Free;
};
typedef struct _XRDP_MSG_DEFINITION XRDP_MSG_DEFINITION;

#define XRDP_ORDER_HEADER_LENGTH	10

int xrdp_read_common_header(wStream* s, XRDP_MSG_COMMON* msg)
{
	Stream_Read_UINT16(s, msg->type);
	Stream_Read_UINT32(s, msg->length);
	Stream_Read_UINT32(s, msg->msgFlags);

	if (msg->msgFlags & XRDP_MSG_FLAG_RECT)
	{
		Stream_Read_UINT32(s, msg->rect.x);
		Stream_Read_UINT32(s, msg->rect.y);
		Stream_Read_UINT32(s, msg->rect.width);
		Stream_Read_UINT32(s, msg->rect.height);
	}

	return 0;
}

int xrdp_write_common_header(wStream* s, XRDP_MSG_COMMON* msg)
{
	if (!s)
	{
		return XRDP_ORDER_HEADER_LENGTH +
			((msg->msgFlags & XRDP_MSG_FLAG_RECT) ? 16 : 0);
	}

	Stream_Write_UINT16(s, msg->type);
	Stream_Write_UINT32(s, msg->length);
	Stream_Write_UINT32(s, msg->msgFlags);

	if (msg->msgFlags & XRDP_MSG_FLAG_RECT)
	{
		Stream_Write_UINT32(s, msg->rect.x);
		Stream_Write_UINT32(s, msg->rect.y);
		Stream_Write_UINT32(s, msg->rect.width);
		Stream_Write_UINT32(s, msg->rect.height);
	}

	return 0;
}

/* Client Messages */

int xrdp_read_event(wStream* s, XRDP_MSG_EVENT* msg)
{
	Stream_Read_UINT32(s, msg->subType);
	Stream_Read_UINT32(s, msg->param1);
	Stream_Read_UINT32(s, msg->param2);
	Stream_Read_UINT32(s, msg->param3);
	Stream_Read_UINT32(s, msg->param4);

	return 0;
}

int xrdp_write_event(wStream* s, XRDP_MSG_EVENT* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 20;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->subType);
	Stream_Write_UINT32(s, msg->param1);
	Stream_Write_UINT32(s, msg->param2);
	Stream_Write_UINT32(s, msg->param3);
	Stream_Write_UINT32(s, msg->param4);

	return 0;
}

int xrdp_read_synchronize_keyboard_event(wStream* s, XRDP_MSG_SYNCHRONIZE_KEYBOARD_EVENT* msg)
{
	Stream_Read_UINT32(s, msg->flags);

	return 0;
}

int xrdp_write_synchronize_keyboard_event(wStream* s, XRDP_MSG_SYNCHRONIZE_KEYBOARD_EVENT* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 4;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->flags);

	return 0;
}

int xrdp_read_scancode_keyboard_event(wStream* s, XRDP_MSG_SCANCODE_KEYBOARD_EVENT* msg)
{
	Stream_Read_UINT32(s, msg->flags);
	Stream_Read_UINT32(s, msg->code);
	Stream_Read_UINT32(s, msg->keyboardType);

	return 0;
}

int xrdp_write_scancode_keyboard_event(wStream* s, XRDP_MSG_SCANCODE_KEYBOARD_EVENT* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 12;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->flags);
	Stream_Write_UINT32(s, msg->code);
	Stream_Write_UINT32(s, msg->keyboardType);

	return 0;
}

int xrdp_read_virtual_keyboard_event(wStream* s, XRDP_MSG_VIRTUAL_KEYBOARD_EVENT* msg)
{
	Stream_Read_UINT32(s, msg->flags);
	Stream_Read_UINT32(s, msg->code);

	return 0;
}

int xrdp_write_virtual_keyboard_event(wStream* s, XRDP_MSG_VIRTUAL_KEYBOARD_EVENT* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 8;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->flags);
	Stream_Write_UINT32(s, msg->code);

	return 0;
}

int xrdp_read_unicode_keyboard_event(wStream* s, XRDP_MSG_UNICODE_KEYBOARD_EVENT* msg)
{
	Stream_Read_UINT32(s, msg->flags);
	Stream_Read_UINT32(s, msg->code);

	return 0;
}

int xrdp_write_unicode_keyboard_event(wStream* s, XRDP_MSG_UNICODE_KEYBOARD_EVENT* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 8;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->flags);
	Stream_Write_UINT32(s, msg->code);

	return 0;
}

int xrdp_read_mouse_event(wStream* s, XRDP_MSG_MOUSE_EVENT* msg)
{
	Stream_Read_UINT32(s, msg->flags);
	Stream_Read_UINT32(s, msg->x);
	Stream_Read_UINT32(s, msg->y);

	return 0;
}

int xrdp_write_mouse_event(wStream* s, XRDP_MSG_MOUSE_EVENT* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 12;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->flags);
	Stream_Write_UINT32(s, msg->x);
	Stream_Write_UINT32(s, msg->y);

	return 0;
}

int xrdp_read_extended_mouse_event(wStream* s, XRDP_MSG_EXTENDED_MOUSE_EVENT* msg)
{
	Stream_Read_UINT32(s, msg->flags);
	Stream_Read_UINT32(s, msg->x);
	Stream_Read_UINT32(s, msg->y);

	return 0;
}

int xrdp_write_extended_mouse_event(wStream* s, XRDP_MSG_EXTENDED_MOUSE_EVENT* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 12;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->flags);
	Stream_Write_UINT32(s, msg->x);
	Stream_Write_UINT32(s, msg->y);

	return 0;
}

int xrdp_read_capabilities(wStream* s, XRDP_MSG_CAPABILITIES* msg)
{
	Stream_Read_UINT32(s, msg->DesktopWidth);
	Stream_Read_UINT32(s, msg->DesktopHeight);
	Stream_Read_UINT32(s, msg->ColorDepth);

	return 0;
}

int xrdp_write_capabilities(wStream* s, XRDP_MSG_CAPABILITIES* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 12;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->DesktopWidth);
	Stream_Write_UINT32(s, msg->DesktopHeight);
	Stream_Write_UINT32(s, msg->ColorDepth);

	return 0;
}

int xrdp_read_refresh_rect(wStream* s, XRDP_MSG_REFRESH_RECT* msg)
{
	int index;

	Stream_Read_UINT16(s, msg->numberOfAreas);

	msg->areasToRefresh = (RECTANGLE_16*) Stream_Pointer(s);

	for (index = 0; index < msg->numberOfAreas; index++)
	{
		Stream_Read_UINT16(s, msg->areasToRefresh[index].left);
		Stream_Read_UINT16(s, msg->areasToRefresh[index].top);
		Stream_Read_UINT16(s, msg->areasToRefresh[index].right);
		Stream_Read_UINT16(s, msg->areasToRefresh[index].bottom);
	}

	return 0;
}

int xrdp_write_refresh_rect(wStream* s, XRDP_MSG_REFRESH_RECT* msg)
{
	int index;

	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 2 + (msg->numberOfAreas * 8);

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT16(s, msg->numberOfAreas);

	for (index = 0; index < msg->numberOfAreas; index++)
	{
		Stream_Write_UINT16(s, msg->areasToRefresh[index].left);
		Stream_Write_UINT16(s, msg->areasToRefresh[index].top);
		Stream_Write_UINT16(s, msg->areasToRefresh[index].right);
		Stream_Write_UINT16(s, msg->areasToRefresh[index].bottom);
	}

	return 0;
}

/**
 * Server Messages
 */

/**
 * BeginUpdate
 */

int xrdp_read_begin_update(wStream* s, XRDP_MSG_BEGIN_UPDATE* msg)
{
	return 0;
}

int xrdp_write_begin_update(wStream* s, XRDP_MSG_BEGIN_UPDATE* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg);

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	return 0;
}

void* xrdp_begin_update_copy(XRDP_MSG_BEGIN_UPDATE* msg)
{
	XRDP_MSG_BEGIN_UPDATE* dup = NULL;

	dup = (XRDP_MSG_BEGIN_UPDATE*) malloc(sizeof(XRDP_MSG_BEGIN_UPDATE));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_BEGIN_UPDATE));

	return (void*) dup;
}

void xrdp_begin_update_free(XRDP_MSG_BEGIN_UPDATE* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_BEGIN_UPDATE_DEFINITION =
{
	sizeof(XRDP_MSG_BEGIN_UPDATE), "BeginUpdate",
	(pXrdpMessageRead) xrdp_read_begin_update,
	(pXrdpMessageWrite) xrdp_write_begin_update,
	(pXrdpMessageCopy) xrdp_begin_update_copy,
	(pXrdpMessageFree) xrdp_begin_update_free
};

/**
 * EndUpdate
 */

int xrdp_read_end_update(wStream* s, XRDP_MSG_END_UPDATE* msg)
{
	return 0;
}

int xrdp_write_end_update(wStream* s, XRDP_MSG_END_UPDATE* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg);

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	return 0;
}

void* xrdp_end_update_copy(XRDP_MSG_END_UPDATE* msg)
{
	XRDP_MSG_END_UPDATE* dup = NULL;

	dup = (XRDP_MSG_END_UPDATE*) malloc(sizeof(XRDP_MSG_END_UPDATE));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_END_UPDATE));

	return (void*) dup;
}

void xrdp_end_update_free(XRDP_MSG_END_UPDATE* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_END_UPDATE_DEFINITION =
{
	sizeof(XRDP_MSG_END_UPDATE), "EndUpdate",
	(pXrdpMessageRead) xrdp_read_end_update,
	(pXrdpMessageWrite) xrdp_write_end_update,
	(pXrdpMessageCopy) xrdp_end_update_copy,
	(pXrdpMessageFree) xrdp_end_update_free
};

/**
 * SetClippingRegion
 */

int xrdp_read_set_clipping_region(wStream* s, XRDP_MSG_SET_CLIPPING_REGION* msg)
{
	Stream_Read_UINT16(s, msg->bNullRegion);
	Stream_Read_UINT16(s, msg->nLeftRect);
	Stream_Read_UINT16(s, msg->nTopRect);
	Stream_Read_UINT16(s, msg->nWidth);
	Stream_Read_UINT16(s, msg->nHeight);

	return 0;
}

int xrdp_write_set_clipping_region(wStream* s, XRDP_MSG_SET_CLIPPING_REGION* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 10;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT16(s, msg->bNullRegion);
	Stream_Write_UINT16(s, msg->nLeftRect);
	Stream_Write_UINT16(s, msg->nTopRect);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);

	return 0;
}

void* xrdp_set_clipping_region_copy(XRDP_MSG_SET_CLIPPING_REGION* msg)
{
	XRDP_MSG_SET_CLIPPING_REGION* dup = NULL;

	dup = (XRDP_MSG_SET_CLIPPING_REGION*) malloc(sizeof(XRDP_MSG_SET_CLIPPING_REGION));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_SET_CLIPPING_REGION));

	return (void*) dup;
}

void xrdp_set_clipping_region_free(XRDP_MSG_SET_CLIPPING_REGION* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_SET_CLIPPING_REGION_DEFINITION =
{
	sizeof(XRDP_MSG_SET_CLIPPING_REGION), "SetClippingRegion",
	(pXrdpMessageRead) xrdp_read_set_clipping_region,
	(pXrdpMessageWrite) xrdp_write_set_clipping_region,
	(pXrdpMessageCopy) xrdp_set_clipping_region_copy,
	(pXrdpMessageFree) xrdp_set_clipping_region_free
};

/**
 * OpaqueRect
 */

int xrdp_read_opaque_rect(wStream* s, XRDP_MSG_OPAQUE_RECT* msg)
{
	Stream_Read_UINT16(s, msg->nLeftRect);
	Stream_Read_UINT16(s, msg->nTopRect);
	Stream_Read_UINT16(s, msg->nWidth);
	Stream_Read_UINT16(s, msg->nHeight);
	Stream_Read_UINT32(s, msg->color);

	return 0;
}

int xrdp_write_opaque_rect(wStream* s, XRDP_MSG_OPAQUE_RECT* msg)
{
	msg->msgFlags = XRDP_MSG_FLAG_RECT;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 12;

	if (!s)
		return msg->length;

	msg->rect.x = msg->nLeftRect;
	msg->rect.y = msg->nTopRect;
	msg->rect.width = msg->nWidth;
	msg->rect.height = msg->nHeight;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT16(s, msg->nLeftRect);
	Stream_Write_UINT16(s, msg->nTopRect);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);
	Stream_Write_UINT32(s, msg->color);

	return 0;
}

void* xrdp_opaque_rect_copy(XRDP_MSG_OPAQUE_RECT* msg)
{
	XRDP_MSG_OPAQUE_RECT* dup = NULL;

	dup = (XRDP_MSG_OPAQUE_RECT*) malloc(sizeof(XRDP_MSG_OPAQUE_RECT));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_OPAQUE_RECT));

	return (void*) dup;
}

void xrdp_opaque_rect_free(XRDP_MSG_OPAQUE_RECT* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_OPAQUE_RECT_DEFINITION =
{
	sizeof(XRDP_MSG_OPAQUE_RECT), "OpaqueRect",
	(pXrdpMessageRead) xrdp_read_opaque_rect,
	(pXrdpMessageWrite) xrdp_write_opaque_rect,
	(pXrdpMessageCopy) xrdp_opaque_rect_copy,
	(pXrdpMessageFree) xrdp_opaque_rect_free
};

/**
 * ScreenBlt
 */

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
	msg->msgFlags = XRDP_MSG_FLAG_RECT;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 12;

	if (!s)
		return msg->length;

	msg->rect.x = msg->nLeftRect;
	msg->rect.y = msg->nTopRect;
	msg->rect.width = msg->nWidth;
	msg->rect.height = msg->nHeight;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT16(s, msg->nLeftRect);
	Stream_Write_UINT16(s, msg->nTopRect);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);
	Stream_Write_UINT16(s, msg->nXSrc);
	Stream_Write_UINT16(s, msg->nYSrc);

	return 0;
}

void* xrdp_screen_blt_copy(XRDP_MSG_SCREEN_BLT* msg)
{
	XRDP_MSG_SCREEN_BLT* dup = NULL;

	dup = (XRDP_MSG_SCREEN_BLT*) malloc(sizeof(XRDP_MSG_SCREEN_BLT));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_SCREEN_BLT));

	return (void*) dup;
}

void xrdp_screen_blt_free(XRDP_MSG_SCREEN_BLT* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_SCREEN_BLT_DEFINITION =
{
	sizeof(XRDP_MSG_SCREEN_BLT), "ScreenBlt",
	(pXrdpMessageRead) xrdp_read_screen_blt,
	(pXrdpMessageWrite) xrdp_write_screen_blt,
	(pXrdpMessageCopy) xrdp_screen_blt_copy,
	(pXrdpMessageFree) xrdp_screen_blt_free
};

/**
 * PaintRect
 */

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
	msg->msgFlags = XRDP_MSG_FLAG_RECT;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 20;

	if (msg->fbSegmentId)
		msg->length += 4;
	else
		msg->length += msg->bitmapDataLength;

	if (!s)
		return msg->length;

	msg->rect.x = msg->nLeftRect;
	msg->rect.y = msg->nTopRect;
	msg->rect.width = msg->nWidth;
	msg->rect.height = msg->nHeight;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

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

void* xrdp_paint_rect_copy(XRDP_MSG_PAINT_RECT* msg)
{
	XRDP_MSG_PAINT_RECT* dup = NULL;

	dup = (XRDP_MSG_PAINT_RECT*) malloc(sizeof(XRDP_MSG_PAINT_RECT));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_PAINT_RECT));

	if (msg->bitmapDataLength)
	{
		dup->bitmapData = (BYTE*) malloc(msg->bitmapDataLength);
		CopyMemory(dup->bitmapData, msg->bitmapData, msg->bitmapDataLength);
	}

	return (void*) dup;
}

void xrdp_paint_rect_free(XRDP_MSG_PAINT_RECT* msg)
{
	if (msg->bitmapDataLength)
		free(msg->bitmapData);

	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_PAINT_RECT_DEFINITION =
{
	sizeof(XRDP_MSG_PAINT_RECT), "PaintRect",
	(pXrdpMessageRead) xrdp_read_paint_rect,
	(pXrdpMessageWrite) xrdp_write_paint_rect,
	(pXrdpMessageCopy) xrdp_paint_rect_copy,
	(pXrdpMessageFree) xrdp_paint_rect_free
};

/**
 * PatBlt
 */

int xrdp_read_patblt(wStream* s, XRDP_MSG_PATBLT* msg)
{
	Stream_Read_UINT32(s, msg->nLeftRect);
	Stream_Read_UINT32(s, msg->nTopRect);
	Stream_Read_UINT32(s, msg->nWidth);
	Stream_Read_UINT32(s, msg->nHeight);
	Stream_Read_UINT32(s, msg->bRop);
	Stream_Read_UINT32(s, msg->backColor);
	Stream_Read_UINT32(s, msg->foreColor);

	Stream_Read_UINT32(s, msg->brush.x);
	Stream_Read_UINT32(s, msg->brush.y);
	Stream_Read_UINT32(s, msg->brush.bpp);
	Stream_Read_UINT32(s, msg->brush.style);
	Stream_Read_UINT32(s, msg->brush.hatch);
	Stream_Read_UINT32(s, msg->brush.index);
	Stream_Read(s, msg->brush.data, 8);

	return 0;
}

int xrdp_write_patblt(wStream* s, XRDP_MSG_PATBLT* msg)
{
	msg->msgFlags = XRDP_MSG_FLAG_RECT;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 60;

	if (!s)
		return msg->length;

	msg->rect.x = msg->nLeftRect;
	msg->rect.y = msg->nTopRect;
	msg->rect.width = msg->nWidth;
	msg->rect.height = msg->nHeight;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->nLeftRect);
	Stream_Write_UINT32(s, msg->nTopRect);
	Stream_Write_UINT32(s, msg->nWidth);
	Stream_Write_UINT32(s, msg->nHeight);
	Stream_Write_UINT32(s, msg->bRop);
	Stream_Write_UINT32(s, msg->backColor);
	Stream_Write_UINT32(s, msg->foreColor);

	Stream_Write_UINT32(s, msg->brush.x);
	Stream_Write_UINT32(s, msg->brush.y);
	Stream_Write_UINT32(s, msg->brush.bpp);
	Stream_Write_UINT32(s, msg->brush.style);
	Stream_Write_UINT32(s, msg->brush.hatch);
	Stream_Write_UINT32(s, msg->brush.index);
	Stream_Write(s, msg->brush.data, 8);

	return 0;
}

void* xrdp_patblt_copy(XRDP_MSG_PATBLT* msg)
{
	XRDP_MSG_PATBLT* dup = NULL;

	dup = (XRDP_MSG_PATBLT*) malloc(sizeof(XRDP_MSG_PATBLT));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_PATBLT));

	return (void*) dup;
}

void xrdp_patblt_free(XRDP_MSG_PATBLT* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_PATBLT_DEFINITION =
{
	sizeof(XRDP_MSG_PATBLT), "PatBlt",
	(pXrdpMessageRead) xrdp_read_patblt,
	(pXrdpMessageWrite) xrdp_write_patblt,
	(pXrdpMessageCopy) xrdp_patblt_copy,
	(pXrdpMessageFree) xrdp_patblt_free
};

/**
 * DstBlt
 */

int xrdp_read_dstblt(wStream* s, XRDP_MSG_DSTBLT* msg)
{
	Stream_Read_UINT32(s, msg->nLeftRect);
	Stream_Read_UINT32(s, msg->nTopRect);
	Stream_Read_UINT32(s, msg->nWidth);
	Stream_Read_UINT32(s, msg->nHeight);
	Stream_Read_UINT32(s, msg->bRop);

	return 0;
}

int xrdp_write_dstblt(wStream* s, XRDP_MSG_DSTBLT* msg)
{
	msg->msgFlags = XRDP_MSG_FLAG_RECT;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 20;

	if (!s)
		return msg->length;

	msg->rect.x = msg->nLeftRect;
	msg->rect.y = msg->nTopRect;
	msg->rect.width = msg->nWidth;
	msg->rect.height = msg->nHeight;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->nLeftRect);
	Stream_Write_UINT32(s, msg->nTopRect);
	Stream_Write_UINT32(s, msg->nWidth);
	Stream_Write_UINT32(s, msg->nHeight);
	Stream_Write_UINT32(s, msg->bRop);

	return 0;
}

void* xrdp_dstblt_copy(XRDP_MSG_DSTBLT* msg)
{
	XRDP_MSG_DSTBLT* dup = NULL;

	dup = (XRDP_MSG_DSTBLT*) malloc(sizeof(XRDP_MSG_DSTBLT));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_DSTBLT));

	return (void*) dup;
}

void xrdp_dstblt_free(XRDP_MSG_DSTBLT* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_DSTBLT_DEFINITION =
{
	sizeof(XRDP_MSG_DSTBLT), "DstBlt",
	(pXrdpMessageRead) xrdp_read_dstblt,
	(pXrdpMessageWrite) xrdp_write_dstblt,
	(pXrdpMessageCopy) xrdp_dstblt_copy,
	(pXrdpMessageFree) xrdp_dstblt_free
};

/**
 * LineTo
 */

int xrdp_read_line_to(wStream* s, XRDP_MSG_LINE_TO* msg)
{
	Stream_Read_UINT32(s, msg->nXStart);
	Stream_Read_UINT32(s, msg->nYStart);
	Stream_Read_UINT32(s, msg->nXEnd);
	Stream_Read_UINT32(s, msg->nYEnd);
	Stream_Read_UINT32(s, msg->bRop2);
	Stream_Read_UINT32(s, msg->penStyle);
	Stream_Read_UINT32(s, msg->penWidth);
	Stream_Read_UINT32(s, msg->penColor);

	return 0;
}

int xrdp_write_line_to(wStream* s, XRDP_MSG_LINE_TO* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 32;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->nXStart);
	Stream_Write_UINT32(s, msg->nYStart);
	Stream_Write_UINT32(s, msg->nXEnd);
	Stream_Write_UINT32(s, msg->nYEnd);
	Stream_Write_UINT32(s, msg->bRop2);
	Stream_Write_UINT32(s, msg->penStyle);
	Stream_Write_UINT32(s, msg->penWidth);
	Stream_Write_UINT32(s, msg->penColor);

	return 0;
}

void* xrdp_line_to_copy(XRDP_MSG_LINE_TO* msg)
{
	XRDP_MSG_LINE_TO* dup = NULL;

	dup = (XRDP_MSG_LINE_TO*) malloc(sizeof(XRDP_MSG_LINE_TO));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_LINE_TO));

	return (void*) dup;
}

void xrdp_line_to_free(XRDP_MSG_LINE_TO* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_LINE_TO_DEFINITION =
{
	sizeof(XRDP_MSG_LINE_TO), "LineTo",
	(pXrdpMessageRead) xrdp_read_line_to,
	(pXrdpMessageWrite) xrdp_write_line_to,
	(pXrdpMessageCopy) xrdp_line_to_copy,
	(pXrdpMessageFree) xrdp_line_to_free
};

/**
 * CreateOffscreenSurface
 */

int xrdp_read_create_offscreen_surface(wStream* s, XRDP_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	Stream_Read_UINT32(s, msg->cacheIndex);
	Stream_Read_UINT16(s, msg->nWidth);
	Stream_Read_UINT16(s, msg->nHeight);

	return 0;
}

int xrdp_write_create_offscreen_surface(wStream* s, XRDP_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 8;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->cacheIndex);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);

	return 0;
}

void* xrdp_create_offscreen_surface_copy(XRDP_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	XRDP_MSG_CREATE_OFFSCREEN_SURFACE* dup = NULL;

	dup = (XRDP_MSG_CREATE_OFFSCREEN_SURFACE*) malloc(sizeof(XRDP_MSG_CREATE_OFFSCREEN_SURFACE));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_CREATE_OFFSCREEN_SURFACE));

	return (void*) dup;
}

void xrdp_create_offscreen_surface_free(XRDP_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_CREATE_OFFSCREEN_SURFACE_DEFINITION =
{
	sizeof(XRDP_MSG_CREATE_OFFSCREEN_SURFACE), "CreateOffscreenSurface",
	(pXrdpMessageRead) xrdp_read_create_offscreen_surface,
	(pXrdpMessageWrite) xrdp_write_create_offscreen_surface,
	(pXrdpMessageCopy) xrdp_create_offscreen_surface_copy,
	(pXrdpMessageFree) xrdp_create_offscreen_surface_free
};

/**
 * SwitchOffscreenSurface
 */

int xrdp_read_switch_offscreen_surface(wStream* s, XRDP_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	Stream_Read_UINT32(s, msg->cacheIndex);

	return 0;
}

int xrdp_write_switch_offscreen_surface(wStream* s, XRDP_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 4;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->cacheIndex);

	return 0;
}

void* xrdp_switch_offscreen_surface_copy(XRDP_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	XRDP_MSG_SWITCH_OFFSCREEN_SURFACE* dup = NULL;

	dup = (XRDP_MSG_SWITCH_OFFSCREEN_SURFACE*) malloc(sizeof(XRDP_MSG_SWITCH_OFFSCREEN_SURFACE));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_SWITCH_OFFSCREEN_SURFACE));

	return (void*) dup;
}

void xrdp_switch_offscreen_surface_free(XRDP_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_SWITCH_OFFSCREEN_SURFACE_DEFINITION =
{
	sizeof(XRDP_MSG_SWITCH_OFFSCREEN_SURFACE), "SwitchOffscreenSurface",
	(pXrdpMessageRead) xrdp_read_switch_offscreen_surface,
	(pXrdpMessageWrite) xrdp_write_switch_offscreen_surface,
	(pXrdpMessageCopy) xrdp_switch_offscreen_surface_copy,
	(pXrdpMessageFree) xrdp_switch_offscreen_surface_free
};

/**
 * DeleteOffscreenSurface
 */

int xrdp_read_delete_offscreen_surface(wStream* s, XRDP_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	Stream_Read_UINT32(s, msg->cacheIndex);

	return 0;
}

int xrdp_write_delete_offscreen_surface(wStream* s, XRDP_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 4;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->cacheIndex);

	return 0;
}

void* xrdp_delete_offscreen_surface_copy(XRDP_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	XRDP_MSG_DELETE_OFFSCREEN_SURFACE* dup = NULL;

	dup = (XRDP_MSG_DELETE_OFFSCREEN_SURFACE*) malloc(sizeof(XRDP_MSG_DELETE_OFFSCREEN_SURFACE));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_DELETE_OFFSCREEN_SURFACE));

	return (void*) dup;
}

void xrdp_delete_offscreen_surface_free(XRDP_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_DELETE_OFFSCREEN_SURFACE_DEFINITION =
{
	sizeof(XRDP_MSG_DELETE_OFFSCREEN_SURFACE), "DeleteOffscreenSurface",
	(pXrdpMessageRead) xrdp_read_delete_offscreen_surface,
	(pXrdpMessageWrite) xrdp_write_delete_offscreen_surface,
	(pXrdpMessageCopy) xrdp_delete_offscreen_surface_copy,
	(pXrdpMessageFree) xrdp_delete_offscreen_surface_free
};

/**
 * PaintOffscreenSurface
 */

int xrdp_read_paint_offscreen_surface(wStream* s, XRDP_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	Stream_Read_UINT32(s, msg->cacheIndex);
	Stream_Read_UINT32(s, msg->nLeftRect);
	Stream_Read_UINT32(s, msg->nTopRect);
	Stream_Read_UINT32(s, msg->nWidth);
	Stream_Read_UINT32(s, msg->nHeight);
	Stream_Read_UINT32(s, msg->nXSrc);
	Stream_Read_UINT32(s, msg->nYSrc);
	Stream_Read_UINT32(s, msg->bRop);

	return 0;
}

int xrdp_write_paint_offscreen_surface(wStream* s, XRDP_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 32;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->cacheIndex);
	Stream_Write_UINT32(s, msg->nLeftRect);
	Stream_Write_UINT32(s, msg->nTopRect);
	Stream_Write_UINT32(s, msg->nWidth);
	Stream_Write_UINT32(s, msg->nHeight);
	Stream_Write_UINT32(s, msg->nXSrc);
	Stream_Write_UINT32(s, msg->nYSrc);
	Stream_Write_UINT32(s, msg->bRop);

	return 0;
}

void* xrdp_paint_offscreen_surface_copy(XRDP_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	XRDP_MSG_PAINT_OFFSCREEN_SURFACE* dup = NULL;

	dup = (XRDP_MSG_PAINT_OFFSCREEN_SURFACE*) malloc(sizeof(XRDP_MSG_PAINT_OFFSCREEN_SURFACE));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_PAINT_OFFSCREEN_SURFACE));

	return (void*) dup;
}

void xrdp_paint_offscreen_surface_free(XRDP_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_PAINT_OFFSCREEN_SURFACE_DEFINITION =
{
	sizeof(XRDP_MSG_PAINT_OFFSCREEN_SURFACE), "PaintOffscreenSurface",
	(pXrdpMessageRead) xrdp_read_paint_offscreen_surface,
	(pXrdpMessageWrite) xrdp_write_paint_offscreen_surface,
	(pXrdpMessageCopy) xrdp_paint_offscreen_surface_copy,
	(pXrdpMessageFree) xrdp_paint_offscreen_surface_free
};

/**
 * SetPalette
 */

int xrdp_read_set_palette(wStream* s, XRDP_MSG_SET_PALETTE* msg)
{
	return 0;
}

int xrdp_write_set_palette(wStream* s, XRDP_MSG_SET_PALETTE* msg)
{
	return 0;
}

void* xrdp_set_palette_copy(XRDP_MSG_SET_PALETTE* msg)
{
	XRDP_MSG_SET_PALETTE* dup = NULL;

	dup = (XRDP_MSG_SET_PALETTE*) malloc(sizeof(XRDP_MSG_SET_PALETTE));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_SET_PALETTE));

	return (void*) dup;
}

void xrdp_set_palette_free(XRDP_MSG_SET_PALETTE* msg)
{
	free(msg);
}
static XRDP_MSG_DEFINITION XRDP_MSG_SET_PALETTE_DEFINITION =
{
	sizeof(XRDP_MSG_SET_PALETTE), "SetPalette",
	(pXrdpMessageRead) xrdp_read_set_palette,
	(pXrdpMessageWrite) xrdp_write_set_palette,
	(pXrdpMessageCopy) xrdp_set_palette_copy,
	(pXrdpMessageFree) xrdp_set_palette_free
};

/**
 * CacheGlyph
 */

int xrdp_read_cache_glyph(wStream* s, XRDP_MSG_CACHE_GLYPH* msg)
{
	return 0;
}

int xrdp_write_cache_glyph(wStream* s, XRDP_MSG_CACHE_GLYPH* msg)
{
	return 0;
}

void* xrdp_cache_glyph_copy(XRDP_MSG_CACHE_GLYPH* msg)
{
	XRDP_MSG_CACHE_GLYPH* dup = NULL;

	dup = (XRDP_MSG_CACHE_GLYPH*) malloc(sizeof(XRDP_MSG_CACHE_GLYPH));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_CACHE_GLYPH));

	return (void*) dup;
}

void xrdp_cache_glyph_free(XRDP_MSG_CACHE_GLYPH* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_CACHE_GLYPH_DEFINITION =
{
	sizeof(XRDP_MSG_CACHE_GLYPH), "CacheGlyph",
	(pXrdpMessageRead) xrdp_read_cache_glyph,
	(pXrdpMessageWrite) xrdp_write_cache_glyph,
	(pXrdpMessageCopy) xrdp_cache_glyph_copy,
	(pXrdpMessageFree) xrdp_cache_glyph_free
};

/**
 * GlyphIndex
 */

int xrdp_read_glyph_index(wStream* s, XRDP_MSG_GLYPH_INDEX* msg)
{
	return 0;
}

int xrdp_write_glyph_index(wStream* s, XRDP_MSG_GLYPH_INDEX* msg)
{
	return 0;
}

void* xrdp_glyph_index_copy(XRDP_MSG_GLYPH_INDEX* msg)
{
	XRDP_MSG_GLYPH_INDEX* dup = NULL;

	dup = (XRDP_MSG_GLYPH_INDEX*) malloc(sizeof(XRDP_MSG_GLYPH_INDEX));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_GLYPH_INDEX));

	return (void*) dup;
}

void xrdp_glyph_index_free(XRDP_MSG_GLYPH_INDEX* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_GLYPH_INDEX_DEFINITION =
{
	sizeof(XRDP_MSG_GLYPH_INDEX), "GlyphIndex",
	(pXrdpMessageRead) xrdp_read_glyph_index,
	(pXrdpMessageWrite) xrdp_write_glyph_index,
	(pXrdpMessageCopy) xrdp_glyph_index_copy,
	(pXrdpMessageFree) xrdp_glyph_index_free
};

/**
 * SetPointer
 */

int xrdp_read_set_pointer(wStream* s, XRDP_MSG_SET_POINTER* msg)
{
	Stream_Read_UINT16(s, msg->xPos);
	Stream_Read_UINT16(s, msg->yPos);
	Stream_Read_UINT16(s, msg->xorBpp);
	Stream_Read_UINT16(s, msg->lengthXorMask);
	Stream_Read_UINT16(s, msg->lengthAndMask);

	Stream_GetPointer(s, msg->xorMaskData);
	Stream_Seek(s, msg->lengthXorMask);

	Stream_GetPointer(s, msg->andMaskData);
	Stream_Seek(s, msg->lengthAndMask);

	return 0;
}

int xrdp_write_set_pointer(wStream* s, XRDP_MSG_SET_POINTER* msg)
{
	if (!msg->xorBpp)
		msg->xorBpp = 24;

	if (!msg->lengthXorMask)
		msg->lengthXorMask = ((msg->xorBpp + 7) / 8) * 32 * 32;

	if (!msg->lengthAndMask)
		msg->lengthAndMask = 32 * (32 / 8);

	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) +
			10 + msg->lengthXorMask + msg->lengthAndMask;

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

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT16(s, msg->xPos);
	Stream_Write_UINT16(s, msg->yPos);
	Stream_Write_UINT16(s, msg->xorBpp);
	Stream_Write_UINT16(s, msg->lengthXorMask);
	Stream_Write_UINT16(s, msg->lengthAndMask);
	Stream_Write(s, msg->xorMaskData, msg->lengthXorMask);
	Stream_Write(s, msg->andMaskData, msg->lengthAndMask);

	return 0;
}

void* xrdp_set_pointer_copy(XRDP_MSG_SET_POINTER* msg)
{
	XRDP_MSG_SET_POINTER* dup = NULL;

	dup = (XRDP_MSG_SET_POINTER*) malloc(sizeof(XRDP_MSG_SET_POINTER));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_SET_POINTER));

	if (dup->andMaskData)
	{
		dup->andMaskData = (BYTE*) malloc(dup->lengthAndMask);
		CopyMemory(dup->andMaskData, msg->andMaskData, dup->lengthAndMask);
	}

	if (dup->xorMaskData)
	{
		dup->xorMaskData = (BYTE*) malloc(dup->lengthXorMask);
		CopyMemory(dup->xorMaskData, msg->xorMaskData, dup->lengthXorMask);
	}

	return (void*) dup;
}

void xrdp_set_pointer_free(XRDP_MSG_SET_POINTER* msg)
{
	if (msg->andMaskData)
		free(msg->andMaskData);

	if (msg->xorMaskData)
		free(msg->xorMaskData);

	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_SET_POINTER_DEFINITION =
{
	sizeof(XRDP_MSG_SET_POINTER), "SetPointer",
	(pXrdpMessageRead) xrdp_read_set_pointer,
	(pXrdpMessageWrite) xrdp_write_set_pointer,
	(pXrdpMessageCopy) xrdp_set_pointer_copy,
	(pXrdpMessageFree) xrdp_set_pointer_free
};

/**
 * SharedFramebuffer
 */

int xrdp_read_shared_framebuffer(wStream* s, XRDP_MSG_SHARED_FRAMEBUFFER* msg)
{
	Stream_Read_UINT32(s, msg->width);
	Stream_Read_UINT32(s, msg->height);
	Stream_Read_UINT32(s, msg->attach);
	Stream_Read_UINT32(s, msg->scanline);
	Stream_Read_UINT32(s, msg->segmentId);
	Stream_Read_UINT32(s, msg->bitsPerPixel);
	Stream_Read_UINT32(s, msg->bytesPerPixel);

	return 0;
}

int xrdp_write_shared_framebuffer(wStream* s, XRDP_MSG_SHARED_FRAMEBUFFER* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 28;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->width);
	Stream_Write_UINT32(s, msg->height);
	Stream_Write_UINT32(s, msg->attach);
	Stream_Write_UINT32(s, msg->scanline);
	Stream_Write_UINT32(s, msg->segmentId);
	Stream_Write_UINT32(s, msg->bitsPerPixel);
	Stream_Write_UINT32(s, msg->bytesPerPixel);

	return 0;
}

void* xrdp_shared_framebuffer_copy(XRDP_MSG_SHARED_FRAMEBUFFER* msg)
{
	XRDP_MSG_SHARED_FRAMEBUFFER* dup = NULL;

	dup = (XRDP_MSG_SHARED_FRAMEBUFFER*) malloc(sizeof(XRDP_MSG_SHARED_FRAMEBUFFER));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_SHARED_FRAMEBUFFER));

	return (void*) dup;
}

void xrdp_shared_framebuffer_free(XRDP_MSG_SHARED_FRAMEBUFFER* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_SHARED_FRAMEBUFFER_DEFINITION =
{
	sizeof(XRDP_MSG_SHARED_FRAMEBUFFER), "SharedFramebuffer",
	(pXrdpMessageRead) xrdp_read_shared_framebuffer,
	(pXrdpMessageWrite) xrdp_write_shared_framebuffer,
	(pXrdpMessageCopy) xrdp_shared_framebuffer_copy,
	(pXrdpMessageFree) xrdp_shared_framebuffer_free
};

/**
 * Beep
 */

int xrdp_read_beep(wStream* s, XRDP_MSG_BEEP* msg)
{
	return 0;
}

int xrdp_write_beep(wStream* s, XRDP_MSG_BEEP* msg)
{
	return 0;
}

void* xrdp_beep_copy(XRDP_MSG_BEEP* msg)
{
	XRDP_MSG_BEEP* dup = NULL;

	dup = (XRDP_MSG_BEEP*) malloc(sizeof(XRDP_MSG_BEEP));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_BEEP));

	return (void*) dup;
}

void xrdp_beep_free(XRDP_MSG_BEEP* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_BEEP_DEFINITION =
{
	sizeof(XRDP_MSG_BEEP), "Beep",
	(pXrdpMessageRead) xrdp_read_beep,
	(pXrdpMessageWrite) xrdp_write_beep,
	(pXrdpMessageCopy) xrdp_beep_copy,
	(pXrdpMessageFree) xrdp_beep_free
};

/**
 * Reset
 */

int xrdp_read_reset(wStream* s, XRDP_MSG_RESET* msg)
{
	return 0;
}

int xrdp_write_reset(wStream* s, XRDP_MSG_RESET* msg)
{
	return 0;
}

void* xrdp_reset_copy(XRDP_MSG_RESET* msg)
{
	XRDP_MSG_RESET* dup = NULL;

	dup = (XRDP_MSG_RESET*) malloc(sizeof(XRDP_MSG_RESET));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_RESET));

	return (void*) dup;
}

void xrdp_reset_free(XRDP_MSG_RESET* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_RESET_DEFINITION =
{
	sizeof(XRDP_MSG_RESET), "Reset",
	(pXrdpMessageRead) xrdp_read_reset,
	(pXrdpMessageWrite) xrdp_write_reset,
	(pXrdpMessageCopy) xrdp_reset_copy,
	(pXrdpMessageFree) xrdp_reset_free
};

/**
 * WindowNewUpdate
 */

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

	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) +
			(5 * 4) + (2 + msg->titleInfo.length) + (12 * 4) +
			(2 + msg->numWindowRects * 8) + (4 + 4) +
			(2 + msg->numVisibilityRects * 8) + 4;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

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

void* xrdp_window_new_update_copy(XRDP_MSG_WINDOW_NEW_UPDATE* msg)
{
	XRDP_MSG_WINDOW_NEW_UPDATE* dup = NULL;

	dup = (XRDP_MSG_WINDOW_NEW_UPDATE*) malloc(sizeof(XRDP_MSG_WINDOW_NEW_UPDATE));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_WINDOW_NEW_UPDATE));

	return (void*) dup;
}

void xrdp_window_new_update_free(XRDP_MSG_WINDOW_NEW_UPDATE* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_WINDOW_NEW_UPDATE_DEFINITION =
{
	sizeof(XRDP_MSG_WINDOW_NEW_UPDATE), "WindowNewUpdate",
	(pXrdpMessageRead) xrdp_read_window_new_update,
	(pXrdpMessageWrite) xrdp_write_window_new_update,
	(pXrdpMessageCopy) xrdp_window_new_update_copy,
	(pXrdpMessageFree) xrdp_window_new_update_free
};

/**
 * WindowDelete
 */

int xrdp_read_window_delete(wStream* s, XRDP_MSG_WINDOW_DELETE* msg)
{
	Stream_Read_UINT32(s, msg->windowId);

	return 0;
}

int xrdp_write_window_delete(wStream* s, XRDP_MSG_WINDOW_DELETE* msg)
{
	msg->msgFlags = 0;
	msg->length = xrdp_write_common_header(NULL, (XRDP_MSG_COMMON*) msg) + 4;

	if (!s)
		return msg->length;

	xrdp_write_common_header(s, (XRDP_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->windowId);

	return 0;
}

void* xrdp_window_delete_copy(XRDP_MSG_WINDOW_DELETE* msg)
{
	XRDP_MSG_WINDOW_DELETE* dup = NULL;

	dup = (XRDP_MSG_WINDOW_DELETE*) malloc(sizeof(XRDP_MSG_WINDOW_DELETE));
	CopyMemory(dup, msg, sizeof(XRDP_MSG_WINDOW_DELETE));

	return (void*) dup;
}

void xrdp_window_delete_free(XRDP_MSG_WINDOW_DELETE* msg)
{
	free(msg);
}

static XRDP_MSG_DEFINITION XRDP_MSG_WINDOW_DELETE_DEFINITION =
{
	sizeof(XRDP_MSG_WINDOW_DELETE), "WindowDelete",
	(pXrdpMessageRead) xrdp_read_window_delete,
	(pXrdpMessageWrite) xrdp_write_window_delete,
	(pXrdpMessageCopy) xrdp_window_delete_copy,
	(pXrdpMessageFree) xrdp_window_delete_free
};

/**
 * Generic Functions
 */

static XRDP_MSG_DEFINITION* XRDP_SERVER_MSG_DEFINITIONS[32] =
{
	NULL, /* 0 */
	&XRDP_MSG_BEGIN_UPDATE_DEFINITION, /* 1 */
	&XRDP_MSG_END_UPDATE_DEFINITION, /* 2 */
	&XRDP_MSG_SET_CLIPPING_REGION_DEFINITION, /* 3 */
	&XRDP_MSG_OPAQUE_RECT_DEFINITION, /* 4 */
	&XRDP_MSG_SCREEN_BLT_DEFINITION, /* 5 */
	&XRDP_MSG_PAINT_RECT_DEFINITION, /* 6 */
	&XRDP_MSG_PATBLT_DEFINITION, /* 7 */
	&XRDP_MSG_DSTBLT_DEFINITION, /* 8 */
	&XRDP_MSG_LINE_TO_DEFINITION, /* 9 */
	&XRDP_MSG_CREATE_OFFSCREEN_SURFACE_DEFINITION, /* 10 */
	&XRDP_MSG_SWITCH_OFFSCREEN_SURFACE_DEFINITION, /* 11 */
	&XRDP_MSG_DELETE_OFFSCREEN_SURFACE_DEFINITION, /* 12 */
	&XRDP_MSG_PAINT_OFFSCREEN_SURFACE_DEFINITION, /* 13 */
	&XRDP_MSG_SET_PALETTE_DEFINITION, /* 14 */
	&XRDP_MSG_CACHE_GLYPH_DEFINITION, /* 15 */
	&XRDP_MSG_GLYPH_INDEX_DEFINITION, /* 16 */
	&XRDP_MSG_SET_POINTER_DEFINITION, /* 17 */
	&XRDP_MSG_SHARED_FRAMEBUFFER_DEFINITION, /* 18 */
	&XRDP_MSG_BEEP_DEFINITION, /* 19 */
	&XRDP_MSG_RESET_DEFINITION, /* 20 */
	&XRDP_MSG_WINDOW_NEW_UPDATE_DEFINITION, /* 21 */
	&XRDP_MSG_WINDOW_DELETE_DEFINITION, /* 22 */
	NULL, /* 23 */
	NULL, /* 24 */
	NULL, /* 25 */
	NULL, /* 26 */
	NULL, /* 27 */
	NULL, /* 28 */
	NULL, /* 29 */
	NULL, /* 30 */
	NULL /* 31 */
};

int xrdp_server_message_size(UINT32 type)
{
	XRDP_MSG_DEFINITION* msgDef;

	msgDef = XRDP_SERVER_MSG_DEFINITIONS[type];

	if (msgDef)
	{
		if (msgDef->Size)
			return msgDef->Size;
	}

	return sizeof(XRDP_MSG_SERVER);
}

char* xrdp_server_message_name(UINT32 type)
{
	XRDP_MSG_DEFINITION* msgDef;

	msgDef = XRDP_SERVER_MSG_DEFINITIONS[type];

	if (msgDef)
	{
		if (msgDef->Name)
			return (char*) msgDef->Name;
	}

	return "Unknown";
}

int xrdp_server_message_read(wStream* s, XRDP_MSG_COMMON* msg)
{
	int status = 0;
	XRDP_MSG_DEFINITION* msgDef;

	msgDef = XRDP_SERVER_MSG_DEFINITIONS[msg->type];

	if (msgDef)
	{
		if (msgDef->Read)
			status = msgDef->Read(s, msg);
	}

	return status;
}

int xrdp_server_message_write(wStream* s, XRDP_MSG_COMMON* msg)
{
	XRDP_MSG_DEFINITION* msgDef;

	msgDef = XRDP_SERVER_MSG_DEFINITIONS[msg->type];

	if (msgDef)
	{
		if (msgDef->Write)
			msgDef->Write(s, msg);
	}

	return msg->length;
}

void* xrdp_server_message_copy(XRDP_MSG_COMMON* msg)
{
	void* dup = NULL;
	XRDP_MSG_DEFINITION* msgDef;

	msgDef = XRDP_SERVER_MSG_DEFINITIONS[msg->type];

	if (msgDef)
	{
		if (msgDef->Copy)
			dup = msgDef->Copy(msg);
	}

	return dup;
}

void xrdp_server_message_free(XRDP_MSG_COMMON* msg)
{
	XRDP_MSG_DEFINITION* msgDef;

	msgDef = XRDP_SERVER_MSG_DEFINITIONS[msg->type];

	if (msgDef)
	{
		if (msgDef->Free)
			msgDef->Free(msg);
	}
}
