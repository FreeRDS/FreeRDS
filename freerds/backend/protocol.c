/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2013-2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#include <freerds/backend.h>

#include "protocol.h"

typedef int (*pRdsMessageRead)(wStream* s, RDS_MSG_COMMON* msg);
typedef int (*pRdsMessageWrite)(wStream* s, RDS_MSG_COMMON* msg);
typedef void* (*pRdsMessageCopy)(RDS_MSG_COMMON* msg);
typedef void (*pRdsMessageFree)(RDS_MSG_COMMON* msg);

struct _RDS_MSG_DEFINITION
{
	int Size;
	const char* Name;
	pRdsMessageRead Read;
	pRdsMessageWrite Write;
	pRdsMessageCopy Copy;
	pRdsMessageFree Free;
};
typedef struct _RDS_MSG_DEFINITION RDS_MSG_DEFINITION;

UINT32 freerds_peek_common_header_length(BYTE* data)
{
	UINT32 length;
	length = *((UINT32*) &(data[2]));
	return length;
}

int freerds_read_common_header(wStream* s, RDS_MSG_COMMON* msg)
{
	Stream_Read_UINT16(s, msg->type);
	Stream_Read_UINT32(s, msg->length);
	Stream_Read_UINT32(s, msg->msgFlags);

	if (msg->msgFlags & RDS_MSG_FLAG_RECT)
	{
		Stream_Read_UINT32(s, msg->rect.x);
		Stream_Read_UINT32(s, msg->rect.y);
		Stream_Read_UINT32(s, msg->rect.width);
		Stream_Read_UINT32(s, msg->rect.height);
	}

	return 0;
}

int freerds_write_common_header(wStream* s, RDS_MSG_COMMON* msg)
{
	if (!s)
	{
		return RDS_ORDER_HEADER_LENGTH +
			((msg->msgFlags & RDS_MSG_FLAG_RECT) ? 16 : 0);
	}

	Stream_Write_UINT16(s, msg->type);
	Stream_Write_UINT32(s, msg->length);
	Stream_Write_UINT32(s, msg->msgFlags);

	if (msg->msgFlags & RDS_MSG_FLAG_RECT)
	{
		Stream_Write_UINT32(s, msg->rect.x);
		Stream_Write_UINT32(s, msg->rect.y);
		Stream_Write_UINT32(s, msg->rect.width);
		Stream_Write_UINT32(s, msg->rect.height);
	}

	return 0;
}

/* Client Messages */

int freerds_read_synchronize_keyboard_event(wStream* s, RDS_MSG_SYNCHRONIZE_KEYBOARD_EVENT* msg)
{
	if (Stream_GetRemainingLength(s) < 4)
		return -1;
	Stream_Read_UINT32(s, msg->flags);

	return 0;
}

int freerds_write_synchronize_keyboard_event(wStream* s, RDS_MSG_SYNCHRONIZE_KEYBOARD_EVENT* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 4;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->flags);

	return 0;
}

int freerds_read_scancode_keyboard_event(wStream* s, RDS_MSG_SCANCODE_KEYBOARD_EVENT* msg)
{
	if (Stream_GetRemainingLength(s) < 12)
		return -1;
	Stream_Read_UINT32(s, msg->flags);
	Stream_Read_UINT32(s, msg->code);
	Stream_Read_UINT32(s, msg->keyboardType);

	return 0;
}

int freerds_write_scancode_keyboard_event(wStream* s, RDS_MSG_SCANCODE_KEYBOARD_EVENT* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 12;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->flags);
	Stream_Write_UINT32(s, msg->code);
	Stream_Write_UINT32(s, msg->keyboardType);

	return 0;
}

int freerds_read_virtual_keyboard_event(wStream* s, RDS_MSG_VIRTUAL_KEYBOARD_EVENT* msg)
{
	if (Stream_GetRemainingLength(s) < 8)
		return -1;
	Stream_Read_UINT32(s, msg->flags);
	Stream_Read_UINT32(s, msg->code);

	return 0;
}

int freerds_write_virtual_keyboard_event(wStream* s, RDS_MSG_VIRTUAL_KEYBOARD_EVENT* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 8;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->flags);
	Stream_Write_UINT32(s, msg->code);

	return 0;
}

int freerds_read_unicode_keyboard_event(wStream* s, RDS_MSG_UNICODE_KEYBOARD_EVENT* msg)
{
	if (Stream_GetRemainingLength(s) < 8)
		return -1;
	Stream_Read_UINT32(s, msg->flags);
	Stream_Read_UINT32(s, msg->code);

	return 0;
}

int freerds_write_unicode_keyboard_event(wStream* s, RDS_MSG_UNICODE_KEYBOARD_EVENT* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 8;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->flags);
	Stream_Write_UINT32(s, msg->code);

	return 0;
}

int freerds_read_mouse_event(wStream* s, RDS_MSG_MOUSE_EVENT* msg)
{
	if (Stream_GetRemainingLength(s) < 12)
		return -1;
	Stream_Read_UINT32(s, msg->flags);
	Stream_Read_UINT32(s, msg->x);
	Stream_Read_UINT32(s, msg->y);

	return 0;
}

int freerds_write_mouse_event(wStream* s, RDS_MSG_MOUSE_EVENT* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 12;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->flags);
	Stream_Write_UINT32(s, msg->x);
	Stream_Write_UINT32(s, msg->y);

	return 0;
}

int freerds_read_extended_mouse_event(wStream* s, RDS_MSG_EXTENDED_MOUSE_EVENT* msg)
{
	if (Stream_GetRemainingLength(s) < 12)
		return -1;
	Stream_Read_UINT32(s, msg->flags);
	Stream_Read_UINT32(s, msg->x);
	Stream_Read_UINT32(s, msg->y);

	return 0;
}

int freerds_write_extended_mouse_event(wStream* s, RDS_MSG_EXTENDED_MOUSE_EVENT* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 12;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->flags);
	Stream_Write_UINT32(s, msg->x);
	Stream_Write_UINT32(s, msg->y);

	return 0;
}

int freerds_read_vblank_event(wStream* s, RDS_MSG_VBLANK_EVENT* msg)
{
	return 0;
}

int freerds_write_vblank_event(wStream* s, RDS_MSG_VBLANK_EVENT* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON *)msg);

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	return 0;
}


int freerds_read_capabilities(wStream* s, RDS_MSG_CAPABILITIES* msg)
{
	if (Stream_GetRemainingLength(s) < 28)
		return -1;
	Stream_Read_UINT32(s, msg->Version);
	Stream_Read_UINT32(s, msg->DesktopWidth);
	Stream_Read_UINT32(s, msg->DesktopHeight);
	Stream_Read_UINT32(s, msg->ColorDepth);
	Stream_Read_UINT32(s, msg->KeyboardLayout);
	Stream_Read_UINT32(s, msg->KeyboardType);
	Stream_Read_UINT32(s, msg->KeyboardSubType);

	return 0;
}

int freerds_write_capabilities(wStream* s, RDS_MSG_CAPABILITIES* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 28;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->Version);
	Stream_Write_UINT32(s, msg->DesktopWidth);
	Stream_Write_UINT32(s, msg->DesktopHeight);
	Stream_Write_UINT32(s, msg->ColorDepth);
	Stream_Write_UINT32(s, msg->KeyboardLayout);
	Stream_Write_UINT32(s, msg->KeyboardType);
	Stream_Write_UINT32(s, msg->KeyboardSubType);

	return 0;
}

int freerds_read_refresh_rect(wStream* s, RDS_MSG_REFRESH_RECT* msg)
{
	int index;

	if (Stream_GetRemainingLength(s) < 2)
		return -1;
	Stream_Read_UINT16(s, msg->numberOfAreas);

	msg->areasToRefresh = (RECTANGLE_16*) Stream_Pointer(s);
	if (Stream_GetRemainingLength(s) < 8 * msg->numberOfAreas)
		return -1;

	for (index = 0; index < msg->numberOfAreas; index++)
	{
		Stream_Read_UINT16(s, msg->areasToRefresh[index].left);
		Stream_Read_UINT16(s, msg->areasToRefresh[index].top);
		Stream_Read_UINT16(s, msg->areasToRefresh[index].right);
		Stream_Read_UINT16(s, msg->areasToRefresh[index].bottom);
	}

	return 0;
}

int freerds_write_refresh_rect(wStream* s, RDS_MSG_REFRESH_RECT* msg)
{
	int index;

	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 2 + (msg->numberOfAreas * 8);

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

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

int freerds_read_suppress_output(wStream* s, RDS_MSG_SUPPRESS_OUTPUT* msg)
{
	if (Stream_GetRemainingLength(s) < 4)
		return -1;

	Stream_Read_UINT32(s, msg->activeOutput);
	return 0;
}

int freerds_write_suppress_output(wStream* s, RDS_MSG_SUPPRESS_OUTPUT* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(0, (RDS_MSG_COMMON*) msg) + 4;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->activeOutput);
	return 0;
}


/**
 * Server Messages
 */

/**
 * BeginUpdate
 */

int freerds_read_begin_update(wStream* s, RDS_MSG_BEGIN_UPDATE* msg)
{
	return 0;
}

int freerds_write_begin_update(wStream* s, RDS_MSG_BEGIN_UPDATE* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg);

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	return 0;
}

void* freerds_begin_update_copy(RDS_MSG_BEGIN_UPDATE* msg)
{
	RDS_MSG_BEGIN_UPDATE* dup = NULL;

	dup = (RDS_MSG_BEGIN_UPDATE*) malloc(sizeof(RDS_MSG_BEGIN_UPDATE));
	CopyMemory(dup, msg, sizeof(RDS_MSG_BEGIN_UPDATE));

	return (void*) dup;
}

void freerds_begin_update_free(RDS_MSG_BEGIN_UPDATE* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_BEGIN_UPDATE_DEFINITION =
{
	sizeof(RDS_MSG_BEGIN_UPDATE), "BeginUpdate",
	(pRdsMessageRead) freerds_read_begin_update,
	(pRdsMessageWrite) freerds_write_begin_update,
	(pRdsMessageCopy) freerds_begin_update_copy,
	(pRdsMessageFree) freerds_begin_update_free
};

/**
 * EndUpdate
 */

int freerds_read_end_update(wStream* s, RDS_MSG_END_UPDATE* msg)
{
	return 0;
}

int freerds_write_end_update(wStream* s, RDS_MSG_END_UPDATE* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg);

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	return 0;
}

void* freerds_end_update_copy(RDS_MSG_END_UPDATE* msg)
{
	RDS_MSG_END_UPDATE* dup = NULL;

	dup = (RDS_MSG_END_UPDATE*) malloc(sizeof(RDS_MSG_END_UPDATE));
	CopyMemory(dup, msg, sizeof(RDS_MSG_END_UPDATE));

	return (void*) dup;
}

void freerds_end_update_free(RDS_MSG_END_UPDATE* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_END_UPDATE_DEFINITION =
{
	sizeof(RDS_MSG_END_UPDATE), "EndUpdate",
	(pRdsMessageRead) freerds_read_end_update,
	(pRdsMessageWrite) freerds_write_end_update,
	(pRdsMessageCopy) freerds_end_update_copy,
	(pRdsMessageFree) freerds_end_update_free
};

/**
 * SetClippingRegion
 */

int freerds_read_set_clipping_region(wStream* s, RDS_MSG_SET_CLIPPING_REGION* msg)
{
	if (Stream_GetRemainingLength(s) < 10)
		return -1;

	Stream_Read_UINT16(s, msg->bNullRegion);
	Stream_Read_UINT16(s, msg->nLeftRect);
	Stream_Read_UINT16(s, msg->nTopRect);
	Stream_Read_UINT16(s, msg->nWidth);
	Stream_Read_UINT16(s, msg->nHeight);

	return 0;
}

int freerds_write_set_clipping_region(wStream* s, RDS_MSG_SET_CLIPPING_REGION* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 10;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT16(s, msg->bNullRegion);
	Stream_Write_UINT16(s, msg->nLeftRect);
	Stream_Write_UINT16(s, msg->nTopRect);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);

	return 0;
}

void* freerds_set_clipping_region_copy(RDS_MSG_SET_CLIPPING_REGION* msg)
{
	RDS_MSG_SET_CLIPPING_REGION* dup = NULL;

	dup = (RDS_MSG_SET_CLIPPING_REGION*) malloc(sizeof(RDS_MSG_SET_CLIPPING_REGION));
	CopyMemory(dup, msg, sizeof(RDS_MSG_SET_CLIPPING_REGION));

	return (void*) dup;
}

void freerds_set_clipping_region_free(RDS_MSG_SET_CLIPPING_REGION* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_SET_CLIPPING_REGION_DEFINITION =
{
	sizeof(RDS_MSG_SET_CLIPPING_REGION), "SetClippingRegion",
	(pRdsMessageRead) freerds_read_set_clipping_region,
	(pRdsMessageWrite) freerds_write_set_clipping_region,
	(pRdsMessageCopy) freerds_set_clipping_region_copy,
	(pRdsMessageFree) freerds_set_clipping_region_free
};

/**
 * OpaqueRect
 */

int freerds_read_opaque_rect(wStream* s, RDS_MSG_OPAQUE_RECT* msg)
{
	if (Stream_GetRemainingLength(s) < 12)
		return -1;

	Stream_Read_UINT16(s, msg->nLeftRect);
	Stream_Read_UINT16(s, msg->nTopRect);
	Stream_Read_UINT16(s, msg->nWidth);
	Stream_Read_UINT16(s, msg->nHeight);
	Stream_Read_UINT32(s, msg->color);

	return 0;
}

int freerds_write_opaque_rect(wStream* s, RDS_MSG_OPAQUE_RECT* msg)
{
	msg->msgFlags = RDS_MSG_FLAG_RECT;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 12;

	if (!s)
		return msg->length;

	msg->rect.x = msg->nLeftRect;
	msg->rect.y = msg->nTopRect;
	msg->rect.width = msg->nWidth;
	msg->rect.height = msg->nHeight;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT16(s, msg->nLeftRect);
	Stream_Write_UINT16(s, msg->nTopRect);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);
	Stream_Write_UINT32(s, msg->color);

	return 0;
}

void* freerds_opaque_rect_copy(RDS_MSG_OPAQUE_RECT* msg)
{
	RDS_MSG_OPAQUE_RECT* dup = NULL;

	dup = (RDS_MSG_OPAQUE_RECT*) malloc(sizeof(RDS_MSG_OPAQUE_RECT));
	CopyMemory(dup, msg, sizeof(RDS_MSG_OPAQUE_RECT));

	return (void*) dup;
}

void freerds_opaque_rect_free(RDS_MSG_OPAQUE_RECT* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_OPAQUE_RECT_DEFINITION =
{
	sizeof(RDS_MSG_OPAQUE_RECT), "OpaqueRect",
	(pRdsMessageRead) freerds_read_opaque_rect,
	(pRdsMessageWrite) freerds_write_opaque_rect,
	(pRdsMessageCopy) freerds_opaque_rect_copy,
	(pRdsMessageFree) freerds_opaque_rect_free
};

/**
 * ScreenBlt
 */

int freerds_read_screen_blt(wStream* s, RDS_MSG_SCREEN_BLT* msg)
{
	if (Stream_GetRemainingLength(s) < 12)
		return -1;

	Stream_Read_UINT16(s, msg->nLeftRect);
	Stream_Read_UINT16(s, msg->nTopRect);
	Stream_Read_UINT16(s, msg->nWidth);
	Stream_Read_UINT16(s, msg->nHeight);
	Stream_Read_UINT16(s, msg->nXSrc);
	Stream_Read_UINT16(s, msg->nYSrc);

	return 0;
}

int freerds_write_screen_blt(wStream* s, RDS_MSG_SCREEN_BLT* msg)
{
	msg->msgFlags = RDS_MSG_FLAG_RECT;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 12;

	if (!s)
		return msg->length;

	msg->rect.x = msg->nLeftRect;
	msg->rect.y = msg->nTopRect;
	msg->rect.width = msg->nWidth;
	msg->rect.height = msg->nHeight;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT16(s, msg->nLeftRect);
	Stream_Write_UINT16(s, msg->nTopRect);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);
	Stream_Write_UINT16(s, msg->nXSrc);
	Stream_Write_UINT16(s, msg->nYSrc);

	return 0;
}

void* freerds_screen_blt_copy(RDS_MSG_SCREEN_BLT* msg)
{
	RDS_MSG_SCREEN_BLT* dup = NULL;

	dup = (RDS_MSG_SCREEN_BLT*) malloc(sizeof(RDS_MSG_SCREEN_BLT));
	CopyMemory(dup, msg, sizeof(RDS_MSG_SCREEN_BLT));

	return (void*) dup;
}

void freerds_screen_blt_free(RDS_MSG_SCREEN_BLT* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_SCREEN_BLT_DEFINITION =
{
	sizeof(RDS_MSG_SCREEN_BLT), "ScreenBlt",
	(pRdsMessageRead) freerds_read_screen_blt,
	(pRdsMessageWrite) freerds_write_screen_blt,
	(pRdsMessageCopy) freerds_screen_blt_copy,
	(pRdsMessageFree) freerds_screen_blt_free
};

/**
 * PaintRect
 */

int freerds_read_paint_rect(wStream* s, RDS_MSG_PAINT_RECT* msg)
{
	if (Stream_GetRemainingLength(s) < 12)
		return -1;

	Stream_Read_UINT16(s, msg->nLeftRect);
	Stream_Read_UINT16(s, msg->nTopRect);
	Stream_Read_UINT16(s, msg->nWidth);
	Stream_Read_UINT16(s, msg->nHeight);
	Stream_Read_UINT32(s, msg->bitmapDataLength);

	if (msg->bitmapDataLength)
	{
		if (Stream_GetRemainingLength(s) < msg->bitmapDataLength)
			return -1;

		Stream_GetPointer(s, msg->bitmapData);
		Stream_Seek(s, msg->bitmapDataLength);
	}
	else
	{
		if (Stream_GetRemainingLength(s) < 4)
			return -1;
		Stream_Read_UINT32(s, msg->fbSegmentId);
	}

	if (Stream_GetRemainingLength(s) < 8)
		return -1;
	Stream_Read_UINT16(s, msg->nWidth);
	Stream_Read_UINT16(s, msg->nHeight);
	Stream_Read_UINT16(s, msg->nXSrc);
	Stream_Read_UINT16(s, msg->nYSrc);

	return 0;
}

int freerds_write_paint_rect(wStream* s, RDS_MSG_PAINT_RECT* msg)
{
	msg->msgFlags = RDS_MSG_FLAG_RECT;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 20;

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

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

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

void* freerds_paint_rect_copy(RDS_MSG_PAINT_RECT* msg)
{
	RDS_MSG_PAINT_RECT* dup = NULL;

	dup = (RDS_MSG_PAINT_RECT*) malloc(sizeof(RDS_MSG_PAINT_RECT));
	CopyMemory(dup, msg, sizeof(RDS_MSG_PAINT_RECT));

	if (msg->bitmapDataLength)
	{
		dup->bitmapData = (BYTE*) malloc(msg->bitmapDataLength);
		CopyMemory(dup->bitmapData, msg->bitmapData, msg->bitmapDataLength);
	}

	return (void*) dup;
}

void freerds_paint_rect_free(RDS_MSG_PAINT_RECT* msg)
{
	if (msg->bitmapDataLength)
		free(msg->bitmapData);

	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_PAINT_RECT_DEFINITION =
{
	sizeof(RDS_MSG_PAINT_RECT), "PaintRect",
	(pRdsMessageRead) freerds_read_paint_rect,
	(pRdsMessageWrite) freerds_write_paint_rect,
	(pRdsMessageCopy) freerds_paint_rect_copy,
	(pRdsMessageFree) freerds_paint_rect_free
};

/**
 * PatBlt
 */

int freerds_read_patblt(wStream* s, RDS_MSG_PATBLT* msg)
{
	if (Stream_GetRemainingLength(s) < 60)
		return -1;

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

int freerds_write_patblt(wStream* s, RDS_MSG_PATBLT* msg)
{
	msg->msgFlags = RDS_MSG_FLAG_RECT;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 60;

	if (!s)
		return msg->length;

	msg->rect.x = msg->nLeftRect;
	msg->rect.y = msg->nTopRect;
	msg->rect.width = msg->nWidth;
	msg->rect.height = msg->nHeight;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

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

void* freerds_patblt_copy(RDS_MSG_PATBLT* msg)
{
	RDS_MSG_PATBLT* dup = NULL;

	dup = (RDS_MSG_PATBLT*) malloc(sizeof(RDS_MSG_PATBLT));
	CopyMemory(dup, msg, sizeof(RDS_MSG_PATBLT));

	return (void*) dup;
}

void freerds_patblt_free(RDS_MSG_PATBLT* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_PATBLT_DEFINITION =
{
	sizeof(RDS_MSG_PATBLT), "PatBlt",
	(pRdsMessageRead) freerds_read_patblt,
	(pRdsMessageWrite) freerds_write_patblt,
	(pRdsMessageCopy) freerds_patblt_copy,
	(pRdsMessageFree) freerds_patblt_free
};

/**
 * DstBlt
 */

int freerds_read_dstblt(wStream* s, RDS_MSG_DSTBLT* msg)
{
	if (Stream_GetRemainingLength(s) < 20)
		return -1;

	Stream_Read_UINT32(s, msg->nLeftRect);
	Stream_Read_UINT32(s, msg->nTopRect);
	Stream_Read_UINT32(s, msg->nWidth);
	Stream_Read_UINT32(s, msg->nHeight);
	Stream_Read_UINT32(s, msg->bRop);

	return 0;
}

int freerds_write_dstblt(wStream* s, RDS_MSG_DSTBLT* msg)
{
	msg->msgFlags = RDS_MSG_FLAG_RECT;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 20;

	if (!s)
		return msg->length;

	msg->rect.x = msg->nLeftRect;
	msg->rect.y = msg->nTopRect;
	msg->rect.width = msg->nWidth;
	msg->rect.height = msg->nHeight;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->nLeftRect);
	Stream_Write_UINT32(s, msg->nTopRect);
	Stream_Write_UINT32(s, msg->nWidth);
	Stream_Write_UINT32(s, msg->nHeight);
	Stream_Write_UINT32(s, msg->bRop);

	return 0;
}

void* freerds_dstblt_copy(RDS_MSG_DSTBLT* msg)
{
	RDS_MSG_DSTBLT* dup = NULL;

	dup = (RDS_MSG_DSTBLT*) malloc(sizeof(RDS_MSG_DSTBLT));
	CopyMemory(dup, msg, sizeof(RDS_MSG_DSTBLT));

	return (void*) dup;
}

void freerds_dstblt_free(RDS_MSG_DSTBLT* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_DSTBLT_DEFINITION =
{
	sizeof(RDS_MSG_DSTBLT), "DstBlt",
	(pRdsMessageRead) freerds_read_dstblt,
	(pRdsMessageWrite) freerds_write_dstblt,
	(pRdsMessageCopy) freerds_dstblt_copy,
	(pRdsMessageFree) freerds_dstblt_free
};

/**
 * LineTo
 */

int freerds_read_line_to(wStream* s, RDS_MSG_LINE_TO* msg)
{
	if (Stream_GetRemainingLength(s) < 8 * 4)
		return -1;

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

int freerds_write_line_to(wStream* s, RDS_MSG_LINE_TO* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 32;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

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

void* freerds_line_to_copy(RDS_MSG_LINE_TO* msg)
{
	RDS_MSG_LINE_TO* dup = NULL;

	dup = (RDS_MSG_LINE_TO*) malloc(sizeof(RDS_MSG_LINE_TO));
	CopyMemory(dup, msg, sizeof(RDS_MSG_LINE_TO));

	return (void*) dup;
}

void freerds_line_to_free(RDS_MSG_LINE_TO* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_LINE_TO_DEFINITION =
{
	sizeof(RDS_MSG_LINE_TO), "LineTo",
	(pRdsMessageRead) freerds_read_line_to,
	(pRdsMessageWrite) freerds_write_line_to,
	(pRdsMessageCopy) freerds_line_to_copy,
	(pRdsMessageFree) freerds_line_to_free
};

/**
 * CreateOffscreenSurface
 */

int freerds_read_create_offscreen_surface(wStream* s, RDS_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	if (Stream_GetRemainingLength(s) < 8)
		return -1;

	Stream_Read_UINT32(s, msg->cacheIndex);
	Stream_Read_UINT16(s, msg->nWidth);
	Stream_Read_UINT16(s, msg->nHeight);

	return 0;
}

int freerds_write_create_offscreen_surface(wStream* s, RDS_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 8;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->cacheIndex);
	Stream_Write_UINT16(s, msg->nWidth);
	Stream_Write_UINT16(s, msg->nHeight);

	return 0;
}

void* freerds_create_offscreen_surface_copy(RDS_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	RDS_MSG_CREATE_OFFSCREEN_SURFACE* dup = NULL;

	dup = (RDS_MSG_CREATE_OFFSCREEN_SURFACE*) malloc(sizeof(RDS_MSG_CREATE_OFFSCREEN_SURFACE));
	CopyMemory(dup, msg, sizeof(RDS_MSG_CREATE_OFFSCREEN_SURFACE));

	return (void*) dup;
}

void freerds_create_offscreen_surface_free(RDS_MSG_CREATE_OFFSCREEN_SURFACE* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_CREATE_OFFSCREEN_SURFACE_DEFINITION =
{
	sizeof(RDS_MSG_CREATE_OFFSCREEN_SURFACE), "CreateOffscreenSurface",
	(pRdsMessageRead) freerds_read_create_offscreen_surface,
	(pRdsMessageWrite) freerds_write_create_offscreen_surface,
	(pRdsMessageCopy) freerds_create_offscreen_surface_copy,
	(pRdsMessageFree) freerds_create_offscreen_surface_free
};

/**
 * SwitchOffscreenSurface
 */

int freerds_read_switch_offscreen_surface(wStream* s, RDS_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	if (Stream_GetRemainingLength(s) < 4)
		return -1;

	Stream_Read_UINT32(s, msg->cacheIndex);

	return 0;
}

int freerds_write_switch_offscreen_surface(wStream* s, RDS_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 4;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->cacheIndex);

	return 0;
}

void* freerds_switch_offscreen_surface_copy(RDS_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	RDS_MSG_SWITCH_OFFSCREEN_SURFACE* dup = NULL;

	dup = (RDS_MSG_SWITCH_OFFSCREEN_SURFACE*) malloc(sizeof(RDS_MSG_SWITCH_OFFSCREEN_SURFACE));
	CopyMemory(dup, msg, sizeof(RDS_MSG_SWITCH_OFFSCREEN_SURFACE));

	return (void*) dup;
}

void freerds_switch_offscreen_surface_free(RDS_MSG_SWITCH_OFFSCREEN_SURFACE* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_SWITCH_OFFSCREEN_SURFACE_DEFINITION =
{
	sizeof(RDS_MSG_SWITCH_OFFSCREEN_SURFACE), "SwitchOffscreenSurface",
	(pRdsMessageRead) freerds_read_switch_offscreen_surface,
	(pRdsMessageWrite) freerds_write_switch_offscreen_surface,
	(pRdsMessageCopy) freerds_switch_offscreen_surface_copy,
	(pRdsMessageFree) freerds_switch_offscreen_surface_free
};

/**
 * DeleteOffscreenSurface
 */

int freerds_read_delete_offscreen_surface(wStream* s, RDS_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	if (Stream_GetRemainingLength(s) < 4)
		return -1;
	Stream_Read_UINT32(s, msg->cacheIndex);

	return 0;
}

int freerds_write_delete_offscreen_surface(wStream* s, RDS_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 4;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->cacheIndex);

	return 0;
}

void* freerds_delete_offscreen_surface_copy(RDS_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	RDS_MSG_DELETE_OFFSCREEN_SURFACE* dup = NULL;

	dup = (RDS_MSG_DELETE_OFFSCREEN_SURFACE*) malloc(sizeof(RDS_MSG_DELETE_OFFSCREEN_SURFACE));
	CopyMemory(dup, msg, sizeof(RDS_MSG_DELETE_OFFSCREEN_SURFACE));

	return (void*) dup;
}

void freerds_delete_offscreen_surface_free(RDS_MSG_DELETE_OFFSCREEN_SURFACE* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_DELETE_OFFSCREEN_SURFACE_DEFINITION =
{
	sizeof(RDS_MSG_DELETE_OFFSCREEN_SURFACE), "DeleteOffscreenSurface",
	(pRdsMessageRead) freerds_read_delete_offscreen_surface,
	(pRdsMessageWrite) freerds_write_delete_offscreen_surface,
	(pRdsMessageCopy) freerds_delete_offscreen_surface_copy,
	(pRdsMessageFree) freerds_delete_offscreen_surface_free
};

/**
 * PaintOffscreenSurface
 */

int freerds_read_paint_offscreen_surface(wStream* s, RDS_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	if (Stream_GetRemainingLength(s) < 4 * 8)
		return -1;
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

int freerds_write_paint_offscreen_surface(wStream* s, RDS_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 32;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

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

void* freerds_paint_offscreen_surface_copy(RDS_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	RDS_MSG_PAINT_OFFSCREEN_SURFACE* dup = NULL;

	dup = (RDS_MSG_PAINT_OFFSCREEN_SURFACE*) malloc(sizeof(RDS_MSG_PAINT_OFFSCREEN_SURFACE));
	CopyMemory(dup, msg, sizeof(RDS_MSG_PAINT_OFFSCREEN_SURFACE));

	return (void*) dup;
}

void freerds_paint_offscreen_surface_free(RDS_MSG_PAINT_OFFSCREEN_SURFACE* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_PAINT_OFFSCREEN_SURFACE_DEFINITION =
{
	sizeof(RDS_MSG_PAINT_OFFSCREEN_SURFACE), "PaintOffscreenSurface",
	(pRdsMessageRead) freerds_read_paint_offscreen_surface,
	(pRdsMessageWrite) freerds_write_paint_offscreen_surface,
	(pRdsMessageCopy) freerds_paint_offscreen_surface_copy,
	(pRdsMessageFree) freerds_paint_offscreen_surface_free
};

/**
 * SetPalette
 */

int freerds_read_set_palette(wStream* s, RDS_MSG_SET_PALETTE* msg)
{
	return 0;
}

int freerds_write_set_palette(wStream* s, RDS_MSG_SET_PALETTE* msg)
{
	return 0;
}

void* freerds_set_palette_copy(RDS_MSG_SET_PALETTE* msg)
{
	RDS_MSG_SET_PALETTE* dup = NULL;

	dup = (RDS_MSG_SET_PALETTE*) malloc(sizeof(RDS_MSG_SET_PALETTE));
	CopyMemory(dup, msg, sizeof(RDS_MSG_SET_PALETTE));

	return (void*) dup;
}

void freerds_set_palette_free(RDS_MSG_SET_PALETTE* msg)
{
	free(msg);
}
static RDS_MSG_DEFINITION RDS_MSG_SET_PALETTE_DEFINITION =
{
	sizeof(RDS_MSG_SET_PALETTE), "SetPalette",
	(pRdsMessageRead) freerds_read_set_palette,
	(pRdsMessageWrite) freerds_write_set_palette,
	(pRdsMessageCopy) freerds_set_palette_copy,
	(pRdsMessageFree) freerds_set_palette_free
};

/**
 * CacheGlyph
 */

int freerds_read_cache_glyph(wStream* s, RDS_MSG_CACHE_GLYPH* msg)
{
	return 0;
}

int freerds_write_cache_glyph(wStream* s, RDS_MSG_CACHE_GLYPH* msg)
{
	return 0;
}

void* freerds_cache_glyph_copy(RDS_MSG_CACHE_GLYPH* msg)
{
	RDS_MSG_CACHE_GLYPH* dup = NULL;

	dup = (RDS_MSG_CACHE_GLYPH*) malloc(sizeof(RDS_MSG_CACHE_GLYPH));
	CopyMemory(dup, msg, sizeof(RDS_MSG_CACHE_GLYPH));

	return (void*) dup;
}

void freerds_cache_glyph_free(RDS_MSG_CACHE_GLYPH* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_CACHE_GLYPH_DEFINITION =
{
	sizeof(RDS_MSG_CACHE_GLYPH), "CacheGlyph",
	(pRdsMessageRead) freerds_read_cache_glyph,
	(pRdsMessageWrite) freerds_write_cache_glyph,
	(pRdsMessageCopy) freerds_cache_glyph_copy,
	(pRdsMessageFree) freerds_cache_glyph_free
};

/**
 * GlyphIndex
 */

int freerds_read_glyph_index(wStream* s, RDS_MSG_GLYPH_INDEX* msg)
{
	return 0;
}

int freerds_write_glyph_index(wStream* s, RDS_MSG_GLYPH_INDEX* msg)
{
	return 0;
}

void* freerds_glyph_index_copy(RDS_MSG_GLYPH_INDEX* msg)
{
	RDS_MSG_GLYPH_INDEX* dup = NULL;

	dup = (RDS_MSG_GLYPH_INDEX*) malloc(sizeof(RDS_MSG_GLYPH_INDEX));
	CopyMemory(dup, msg, sizeof(RDS_MSG_GLYPH_INDEX));

	return (void*) dup;
}

void freerds_glyph_index_free(RDS_MSG_GLYPH_INDEX* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_GLYPH_INDEX_DEFINITION =
{
	sizeof(RDS_MSG_GLYPH_INDEX), "GlyphIndex",
	(pRdsMessageRead) freerds_read_glyph_index,
	(pRdsMessageWrite) freerds_write_glyph_index,
	(pRdsMessageCopy) freerds_glyph_index_copy,
	(pRdsMessageFree) freerds_glyph_index_free
};

/**
 * SetPointer
 */

int freerds_read_set_pointer(wStream* s, RDS_MSG_SET_POINTER* msg)
{
	if (Stream_GetRemainingLength(s) < 10)
		return -1;

	Stream_Read_UINT16(s, msg->xPos);
	Stream_Read_UINT16(s, msg->yPos);
	Stream_Read_UINT16(s, msg->xorBpp);
	Stream_Read_UINT16(s, msg->lengthXorMask);
	Stream_Read_UINT16(s, msg->lengthAndMask);

	if (Stream_GetRemainingLength(s) < msg->lengthXorMask)
		return -1;
	Stream_GetPointer(s, msg->xorMaskData);
	Stream_Seek(s, msg->lengthXorMask);

	if (Stream_GetRemainingLength(s) < msg->lengthAndMask)
		return -1;
	Stream_GetPointer(s, msg->andMaskData);
	Stream_Seek(s, msg->lengthAndMask);

	return 0;
}

int freerds_write_set_pointer(wStream* s, RDS_MSG_SET_POINTER* msg)
{
	if (!msg->xorBpp)
		msg->xorBpp = 24;

	if (!msg->lengthXorMask)
		msg->lengthXorMask = ((msg->xorBpp + 7) / 8) * 32 * 32;

	if (!msg->lengthAndMask)
		msg->lengthAndMask = 32 * (32 / 8);

	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) +
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

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT16(s, msg->xPos);
	Stream_Write_UINT16(s, msg->yPos);
	Stream_Write_UINT16(s, msg->xorBpp);
	Stream_Write_UINT16(s, msg->lengthXorMask);
	Stream_Write_UINT16(s, msg->lengthAndMask);
	Stream_Write(s, msg->xorMaskData, msg->lengthXorMask);
	Stream_Write(s, msg->andMaskData, msg->lengthAndMask);

	return 0;
}

void* freerds_set_pointer_copy(RDS_MSG_SET_POINTER* msg)
{
	RDS_MSG_SET_POINTER* dup = NULL;

	dup = (RDS_MSG_SET_POINTER*) malloc(sizeof(RDS_MSG_SET_POINTER));
	CopyMemory(dup, msg, sizeof(RDS_MSG_SET_POINTER));

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

void freerds_set_pointer_free(RDS_MSG_SET_POINTER* msg)
{
	if (msg->andMaskData)
		free(msg->andMaskData);

	if (msg->xorMaskData)
		free(msg->xorMaskData);

	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_SET_POINTER_DEFINITION =
{
	sizeof(RDS_MSG_SET_POINTER), "SetPointer",
	(pRdsMessageRead) freerds_read_set_pointer,
	(pRdsMessageWrite) freerds_write_set_pointer,
	(pRdsMessageCopy) freerds_set_pointer_copy,
	(pRdsMessageFree) freerds_set_pointer_free
};

/**
 * SetSystemPointer
 */

int freerds_read_set_system_pointer(wStream* s, RDS_MSG_SET_SYSTEM_POINTER* msg)
{
	if (Stream_GetRemainingLength(s) < 4)
		return -1;

	Stream_Read_UINT32(s, msg->ptrType);
	return 0;
}

int freerds_write_set_system_pointer(wStream* s, RDS_MSG_SET_SYSTEM_POINTER* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) +
			4;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->ptrType);
	return 0;
}

void* freerds_set_system_pointer_copy(RDS_MSG_SET_SYSTEM_POINTER* msg)
{
	RDS_MSG_SET_SYSTEM_POINTER* dup = NULL;

	dup = (RDS_MSG_SET_SYSTEM_POINTER*) malloc(sizeof(RDS_MSG_SET_SYSTEM_POINTER));
	CopyMemory(dup, msg, sizeof(RDS_MSG_SET_SYSTEM_POINTER));

	return (void*) dup;
}

void freerds_set_system_pointer_free(RDS_MSG_SET_SYSTEM_POINTER* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_SET_SYSTEM_POINTER_DEFINITION =
{
	sizeof(RDS_MSG_SET_SYSTEM_POINTER), "SetSystemPointer",
	(pRdsMessageRead) freerds_read_set_system_pointer,
	(pRdsMessageWrite) freerds_write_set_system_pointer,
	(pRdsMessageCopy) freerds_set_system_pointer_copy,
	(pRdsMessageFree) freerds_set_system_pointer_free
};

/**
 * SharedFramebuffer
 */

int freerds_read_shared_framebuffer(wStream* s, RDS_MSG_SHARED_FRAMEBUFFER* msg)
{
	if (Stream_GetRemainingLength(s) < 4 * 7)
		return -1;

	Stream_Read_UINT32(s, msg->flags);
	Stream_Read_UINT32(s, msg->width);
	Stream_Read_UINT32(s, msg->height);
	Stream_Read_UINT32(s, msg->scanline);
	Stream_Read_UINT32(s, msg->segmentId);
	Stream_Read_UINT32(s, msg->bitsPerPixel);
	Stream_Read_UINT32(s, msg->bytesPerPixel);

	return 0;
}

int freerds_write_shared_framebuffer(wStream* s, RDS_MSG_SHARED_FRAMEBUFFER* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 28;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->flags);
	Stream_Write_UINT32(s, msg->width);
	Stream_Write_UINT32(s, msg->height);
	Stream_Write_UINT32(s, msg->scanline);
	Stream_Write_UINT32(s, msg->segmentId);
	Stream_Write_UINT32(s, msg->bitsPerPixel);
	Stream_Write_UINT32(s, msg->bytesPerPixel);

	return 0;
}

void* freerds_shared_framebuffer_copy(RDS_MSG_SHARED_FRAMEBUFFER* msg)
{
	RDS_MSG_SHARED_FRAMEBUFFER* dup = NULL;

	dup = (RDS_MSG_SHARED_FRAMEBUFFER*) malloc(sizeof(RDS_MSG_SHARED_FRAMEBUFFER));
	CopyMemory(dup, msg, sizeof(RDS_MSG_SHARED_FRAMEBUFFER));

	return (void*) dup;
}

void freerds_shared_framebuffer_free(RDS_MSG_SHARED_FRAMEBUFFER* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_SHARED_FRAMEBUFFER_DEFINITION =
{
	sizeof(RDS_MSG_SHARED_FRAMEBUFFER), "SharedFramebuffer",
	(pRdsMessageRead) freerds_read_shared_framebuffer,
	(pRdsMessageWrite) freerds_write_shared_framebuffer,
	(pRdsMessageCopy) freerds_shared_framebuffer_copy,
	(pRdsMessageFree) freerds_shared_framebuffer_free
};

/**
 * Beep
 */

int freerds_read_beep(wStream* s, RDS_MSG_BEEP* msg)
{
	return 0;
}

int freerds_write_beep(wStream* s, RDS_MSG_BEEP* msg)
{
	return 0;
}

void* freerds_beep_copy(RDS_MSG_BEEP* msg)
{
	RDS_MSG_BEEP* dup = NULL;

	dup = (RDS_MSG_BEEP*) malloc(sizeof(RDS_MSG_BEEP));
	CopyMemory(dup, msg, sizeof(RDS_MSG_BEEP));

	return (void*) dup;
}

void freerds_beep_free(RDS_MSG_BEEP* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_BEEP_DEFINITION =
{
	sizeof(RDS_MSG_BEEP), "Beep",
	(pRdsMessageRead) freerds_read_beep,
	(pRdsMessageWrite) freerds_write_beep,
	(pRdsMessageCopy) freerds_beep_copy,
	(pRdsMessageFree) freerds_beep_free
};

/**
 * Reset
 */

int freerds_read_reset(wStream* s, RDS_MSG_RESET* msg)
{
	return 0;
}

int freerds_write_reset(wStream* s, RDS_MSG_RESET* msg)
{
	return 0;
}

void* freerds_reset_copy(RDS_MSG_RESET* msg)
{
	RDS_MSG_RESET* dup = NULL;

	dup = (RDS_MSG_RESET*) malloc(sizeof(RDS_MSG_RESET));
	CopyMemory(dup, msg, sizeof(RDS_MSG_RESET));

	return (void*) dup;
}

void freerds_reset_free(RDS_MSG_RESET* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_RESET_DEFINITION =
{
	sizeof(RDS_MSG_RESET), "Reset",
	(pRdsMessageRead) freerds_read_reset,
	(pRdsMessageWrite) freerds_write_reset,
	(pRdsMessageCopy) freerds_reset_copy,
	(pRdsMessageFree) freerds_reset_free
};

/**
 * WindowNewUpdate
 */

int freerds_read_window_new_update(wStream* s, RDS_MSG_WINDOW_NEW_UPDATE* msg)
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

int freerds_write_window_new_update(wStream* s, RDS_MSG_WINDOW_NEW_UPDATE* msg)
{
	int index;
	UINT32 flags;

	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) +
			(5 * 4) + (2 + msg->titleInfo.length) + (12 * 4) +
			(2 + msg->numWindowRects * 8) + (4 + 4) +
			(2 + msg->numVisibilityRects * 8) + 4;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

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

void* freerds_window_new_update_copy(RDS_MSG_WINDOW_NEW_UPDATE* msg)
{
	RDS_MSG_WINDOW_NEW_UPDATE* dup = NULL;

	dup = (RDS_MSG_WINDOW_NEW_UPDATE*) malloc(sizeof(RDS_MSG_WINDOW_NEW_UPDATE));
	CopyMemory(dup, msg, sizeof(RDS_MSG_WINDOW_NEW_UPDATE));

	return (void*) dup;
}

void freerds_window_new_update_free(RDS_MSG_WINDOW_NEW_UPDATE* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_WINDOW_NEW_UPDATE_DEFINITION =
{
	sizeof(RDS_MSG_WINDOW_NEW_UPDATE), "WindowNewUpdate",
	(pRdsMessageRead) freerds_read_window_new_update,
	(pRdsMessageWrite) freerds_write_window_new_update,
	(pRdsMessageCopy) freerds_window_new_update_copy,
	(pRdsMessageFree) freerds_window_new_update_free
};

/**
 * WindowDelete
 */

int freerds_read_window_delete(wStream* s, RDS_MSG_WINDOW_DELETE* msg)
{
	if (Stream_GetRemainingLength(s) < 4)
		return -1;

	Stream_Read_UINT32(s, msg->windowId);

	return 0;
}

int freerds_write_window_delete(wStream* s, RDS_MSG_WINDOW_DELETE* msg)
{
	msg->msgFlags = 0;
	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 4;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->windowId);

	return 0;
}

void* freerds_window_delete_copy(RDS_MSG_WINDOW_DELETE* msg)
{
	RDS_MSG_WINDOW_DELETE* dup = NULL;

	dup = (RDS_MSG_WINDOW_DELETE*) malloc(sizeof(RDS_MSG_WINDOW_DELETE));
	CopyMemory(dup, msg, sizeof(RDS_MSG_WINDOW_DELETE));

	return (void*) dup;
}

void freerds_window_delete_free(RDS_MSG_WINDOW_DELETE* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_WINDOW_DELETE_DEFINITION =
{
	sizeof(RDS_MSG_WINDOW_DELETE), "WindowDelete",
	(pRdsMessageRead) freerds_read_window_delete,
	(pRdsMessageWrite) freerds_write_window_delete,
	(pRdsMessageCopy) freerds_window_delete_copy,
	(pRdsMessageFree) freerds_window_delete_free
};

/**
 * LogonUser
 */

int freerds_read_logon_user(wStream* s, RDS_MSG_LOGON_USER* msg)
{
	if (Stream_GetRemainingLength(s) < 16)
		return -1;

	Stream_Read_UINT32(s, msg->Flags);
	Stream_Read_UINT32(s, msg->UserLength);
	Stream_Read_UINT32(s, msg->DomainLength);
	Stream_Read_UINT32(s, msg->PasswordLength);

	msg->User = msg->Domain = msg->Password = NULL;

	if (msg->UserLength)
	{
		msg->User = (char*) malloc(msg->UserLength + 1);
		Stream_Read(s, msg->User, msg->UserLength);
		msg->User[msg->UserLength] = '\0';
	}

	if (msg->DomainLength)
	{
		msg->Domain = (char*) malloc(msg->DomainLength + 1);
		Stream_Read(s, msg->Domain, msg->DomainLength);
		msg->Domain[msg->DomainLength] = '\0';
	}

	if (msg->PasswordLength)
	{
		msg->Password = (char*) malloc(msg->PasswordLength + 1);
		Stream_Read(s, msg->Password, msg->PasswordLength);
		msg->Password[msg->PasswordLength] = '\0';
	}

	return 0;
}

int freerds_write_logon_user(wStream* s, RDS_MSG_LOGON_USER* msg)
{
	msg->msgFlags = 0;

	msg->UserLength = msg->DomainLength = msg->PasswordLength = 0;

	if (msg->User)
		msg->UserLength = strlen(msg->User);

	if (msg->Domain)
		msg->DomainLength = strlen(msg->Domain);

	if (msg->Password)
		msg->PasswordLength = strlen(msg->Password);

	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 16 +
			msg->UserLength + msg->DomainLength + msg->PasswordLength;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->Flags);
	Stream_Write_UINT32(s, msg->UserLength);
	Stream_Write_UINT32(s, msg->DomainLength);
	Stream_Write_UINT32(s, msg->PasswordLength);

	if (msg->UserLength)
		Stream_Write(s, msg->User, msg->UserLength);

	if (msg->DomainLength)
		Stream_Write(s, msg->Domain, msg->DomainLength);

	if (msg->PasswordLength)
		Stream_Write(s, msg->Password, msg->PasswordLength);

	return 0;
}

void* freerds_logon_user_copy(RDS_MSG_LOGON_USER* msg)
{
	RDS_MSG_LOGON_USER* dup = NULL;

	dup = (RDS_MSG_LOGON_USER*) malloc(sizeof(RDS_MSG_LOGON_USER));
	CopyMemory(dup, msg, sizeof(RDS_MSG_LOGON_USER));

	if (msg->UserLength)
	{
		dup->User = malloc(msg->UserLength + 1);
		CopyMemory(dup->User, msg->User, msg->UserLength);
		dup->User[msg->UserLength] = '\0';
	}

	if (msg->DomainLength)
	{
		dup->Domain = malloc(msg->DomainLength + 1);
		CopyMemory(dup->Domain, msg->Domain, msg->DomainLength);
		dup->Domain[msg->DomainLength] = '\0';
	}

	if (msg->PasswordLength)
	{
		dup->Password = malloc(msg->PasswordLength + 1);
		CopyMemory(dup->Password, msg->Password, msg->PasswordLength);
		dup->Password[msg->PasswordLength] = '\0';
	}

	return (void*) dup;
}

void freerds_logon_user_free(RDS_MSG_LOGON_USER* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_LOGON_USER_DEFINITION =
{
	sizeof(RDS_MSG_LOGON_USER), "LogonUser",
	(pRdsMessageRead) freerds_read_logon_user,
	(pRdsMessageWrite) freerds_write_logon_user,
	(pRdsMessageCopy) freerds_logon_user_copy,
	(pRdsMessageFree) freerds_logon_user_free
};

/**
 * LogoffUser
 */

int freerds_read_logoff_user(wStream* s, RDS_MSG_LOGOFF_USER* msg)
{
	if (Stream_GetRemainingLength(s) < 4)
		return -1;

	Stream_Read_UINT32(s, msg->Flags);

	return 0;
}

int freerds_write_logoff_user(wStream* s, RDS_MSG_LOGOFF_USER* msg)
{
	msg->msgFlags = 0;

	msg->length = freerds_write_common_header(NULL, (RDS_MSG_COMMON*) msg) + 4;

	if (!s)
		return msg->length;

	freerds_write_common_header(s, (RDS_MSG_COMMON*) msg);

	Stream_Write_UINT32(s, msg->Flags);

	return 0;
}

void* freerds_logoff_user_copy(RDS_MSG_LOGOFF_USER* msg)
{
	RDS_MSG_LOGOFF_USER* dup = NULL;

	dup = (RDS_MSG_LOGOFF_USER*) malloc(sizeof(RDS_MSG_LOGOFF_USER));
	CopyMemory(dup, msg, sizeof(RDS_MSG_LOGOFF_USER));

	return (void*) dup;
}

void freerds_logoff_user_free(RDS_MSG_LOGOFF_USER* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_LOGOFF_USER_DEFINITION =
{
	sizeof(RDS_MSG_LOGOFF_USER), "LogoffUser",
	(pRdsMessageRead) freerds_read_logoff_user,
	(pRdsMessageWrite) freerds_write_logoff_user,
	(pRdsMessageCopy) freerds_logoff_user_copy,
	(pRdsMessageFree) freerds_logoff_user_free
};

/**
 * Generic Functions
 */

void* freerds_suppress_output_copy(RDS_MSG_SUPPRESS_OUTPUT* msg)
{
	RDS_MSG_SUPPRESS_OUTPUT* dup = NULL;

	dup = (RDS_MSG_SUPPRESS_OUTPUT *)malloc(sizeof(RDS_MSG_SUPPRESS_OUTPUT));
	CopyMemory(dup, msg, sizeof(RDS_MSG_SUPPRESS_OUTPUT));

	return (void *)dup;
}

void freerds_suppress_output_free(RDS_MSG_SUPPRESS_OUTPUT* msg)
{
	free(msg);
}

static RDS_MSG_DEFINITION RDS_MSG_SUPPRESS_OUTPUT_DEFINITION =
{
		sizeof(RDS_MSG_SUPPRESS_OUTPUT), "Suppress output",
		(pRdsMessageRead) freerds_read_suppress_output,
		(pRdsMessageWrite) freerds_write_suppress_output,
		(pRdsMessageCopy) freerds_suppress_output_copy,
		(pRdsMessageFree) freerds_suppress_output_free
};



static RDS_MSG_DEFINITION* RDS_CLIENT_MSG_DEFINITIONS[32] =
{
	NULL, /* 0 */
	NULL, /* 1 */
	NULL, /* 2 */
	NULL, /* 3 */
	NULL, /* 4 */
	NULL, /* 5 */
	NULL, /* 6 */
	NULL, /* 7 */
	NULL, /* 8 */
	NULL, /* 9 */
	NULL, /* 10 */
	&RDS_MSG_LOGON_USER_DEFINITION, /* 11 */
	&RDS_MSG_LOGOFF_USER_DEFINITION, /* 12 */
	NULL, /* 13 */
	&RDS_MSG_SUPPRESS_OUTPUT_DEFINITION, /* 14 */
	NULL, /* 15 */
	NULL, /* 16 */
	NULL, /* 17 */
	NULL, /* 18 */
	NULL, /* 19 */
	NULL, /* 20 */
	NULL, /* 21 */
	NULL, /* 22 */
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

int freerds_client_message_size(UINT32 type)
{
	RDS_MSG_DEFINITION* msgDef;

	type -= 100;

	msgDef = RDS_CLIENT_MSG_DEFINITIONS[type];

	if (msgDef)
	{
		if (msgDef->Size)
			return msgDef->Size;
	}

	return sizeof(RDS_MSG_SERVER);
}

char* freerds_client_message_name(UINT32 type)
{
	RDS_MSG_DEFINITION* msgDef;

	type -= 100;

	msgDef = RDS_CLIENT_MSG_DEFINITIONS[type];

	if (msgDef)
	{
		if (msgDef->Name)
			return (char*) msgDef->Name;
	}

	return "Unknown";
}

int freerds_client_message_read(wStream* s, RDS_MSG_COMMON* msg)
{
	int type;
	int status = 0;
	RDS_MSG_DEFINITION* msgDef;

	type = msg->type - 100;
	msgDef = RDS_CLIENT_MSG_DEFINITIONS[type];

	if (msgDef)
	{
		if (msgDef->Read)
			status = msgDef->Read(s, msg);
	}

	return status;
}

int freerds_client_message_write(wStream* s, RDS_MSG_COMMON* msg)
{
	int type;
	RDS_MSG_DEFINITION* msgDef;

	type = msg->type - 100;

	if (type > 31)
	{
		fprintf(stderr, "unable to treat message type %d\n", type);
		return 0;
	}

	msgDef = RDS_CLIENT_MSG_DEFINITIONS[type];

	if (msgDef)
	{
		if (msgDef->Write)
			msgDef->Write(s, msg);
	}

	return msg->length;
}

void* freerds_client_message_copy(RDS_MSG_COMMON* msg)
{
	int type;
	void* dup = NULL;
	RDS_MSG_DEFINITION* msgDef;

	type = msg->type - 100;

	msgDef = RDS_CLIENT_MSG_DEFINITIONS[type];

	if (msgDef)
	{
		if (msgDef->Copy)
			dup = msgDef->Copy(msg);
	}

	return dup;
}

void freerds_client_message_free(RDS_MSG_COMMON* msg)
{
	int type;
	RDS_MSG_DEFINITION* msgDef;

	type = msg->type - 100;

	msgDef = RDS_CLIENT_MSG_DEFINITIONS[msg->type];

	if (msgDef)
	{
		if (msgDef->Free)
			msgDef->Free(msg);
	}
}

/**
 * Server
 */

static RDS_MSG_DEFINITION* RDS_SERVER_MSG_DEFINITIONS[32] =
{
	NULL, /* 0 */
	&RDS_MSG_BEGIN_UPDATE_DEFINITION, /* 1 */
	&RDS_MSG_END_UPDATE_DEFINITION, /* 2 */
	&RDS_MSG_SET_CLIPPING_REGION_DEFINITION, /* 3 */
	&RDS_MSG_OPAQUE_RECT_DEFINITION, /* 4 */
	&RDS_MSG_SCREEN_BLT_DEFINITION, /* 5 */
	&RDS_MSG_PAINT_RECT_DEFINITION, /* 6 */
	&RDS_MSG_PATBLT_DEFINITION, /* 7 */
	&RDS_MSG_DSTBLT_DEFINITION, /* 8 */
	&RDS_MSG_LINE_TO_DEFINITION, /* 9 */
	&RDS_MSG_CREATE_OFFSCREEN_SURFACE_DEFINITION, /* 10 */
	&RDS_MSG_SWITCH_OFFSCREEN_SURFACE_DEFINITION, /* 11 */
	&RDS_MSG_DELETE_OFFSCREEN_SURFACE_DEFINITION, /* 12 */
	&RDS_MSG_PAINT_OFFSCREEN_SURFACE_DEFINITION, /* 13 */
	&RDS_MSG_SET_PALETTE_DEFINITION, /* 14 */
	&RDS_MSG_CACHE_GLYPH_DEFINITION, /* 15 */
	&RDS_MSG_GLYPH_INDEX_DEFINITION, /* 16 */
	&RDS_MSG_SET_POINTER_DEFINITION, /* 17 */
	&RDS_MSG_SHARED_FRAMEBUFFER_DEFINITION, /* 18 */
	&RDS_MSG_BEEP_DEFINITION, /* 19 */
	&RDS_MSG_RESET_DEFINITION, /* 20 */
	&RDS_MSG_WINDOW_NEW_UPDATE_DEFINITION, /* 21 */
	&RDS_MSG_WINDOW_DELETE_DEFINITION, /* 22 */
	&RDS_MSG_SET_SYSTEM_POINTER_DEFINITION, /* 23 */
	&RDS_MSG_LOGON_USER_DEFINITION, /* 24 */
	&RDS_MSG_LOGOFF_USER_DEFINITION, /* 25 */
	NULL, /* 26 */
	NULL, /* 27 */
	NULL, /* 28 */
	NULL, /* 29 */
	NULL, /* 30 */
	NULL /* 31 */
};

int freerds_server_message_size(UINT32 type)
{
	RDS_MSG_DEFINITION* msgDef;

	msgDef = RDS_SERVER_MSG_DEFINITIONS[type];

	if (msgDef)
	{
		if (msgDef->Size)
			return msgDef->Size;
	}

	return sizeof(RDS_MSG_SERVER);
}

char* freerds_server_message_name(UINT32 type)
{
	RDS_MSG_DEFINITION* msgDef;

	msgDef = RDS_SERVER_MSG_DEFINITIONS[type];

	if (msgDef)
	{
		if (msgDef->Name)
			return (char*) msgDef->Name;
	}

	return "Unknown";
}

int freerds_server_message_read(wStream* s, RDS_MSG_COMMON* msg)
{
	int status = -1;
	RDS_MSG_DEFINITION* msgDef;

	msgDef = RDS_SERVER_MSG_DEFINITIONS[msg->type];

	if (msgDef)
	{
		if (msgDef->Read)
			status = msgDef->Read(s, msg);
	}

	return status;
}

int freerds_server_message_write(wStream* s, RDS_MSG_COMMON* msg)
{
	RDS_MSG_DEFINITION* msgDef;

	if (msg->type > 31)
	{
		fprintf(stderr, "unable to treat message type %d\n", msg->type);
		return 0;
	}

	msgDef = RDS_SERVER_MSG_DEFINITIONS[msg->type];

	if (msgDef)
	{
		if (msgDef->Write)
			msgDef->Write(s, msg);
	}

	return msg->length;
}

void* freerds_server_message_copy(RDS_MSG_COMMON* msg)
{
	void* dup = NULL;
	RDS_MSG_DEFINITION* msgDef;

	msgDef = RDS_SERVER_MSG_DEFINITIONS[msg->type];

	if (msgDef)
	{
		if (msgDef->Copy)
			dup = msgDef->Copy(msg);
	}

	return dup;
}

void freerds_server_message_free(RDS_MSG_COMMON* msg)
{
	RDS_MSG_DEFINITION* msgDef;

	msgDef = RDS_SERVER_MSG_DEFINITIONS[msg->type];

	if (msgDef)
	{
		if (msgDef->Free)
			msgDef->Free(msg);
	}
}
