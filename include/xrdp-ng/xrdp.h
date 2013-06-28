/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * xrdp-ng interface
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

#ifndef XRDP_NG_H
#define XRDP_NG_H

#include <freerdp/api.h>
#include <freerdp/freerdp.h>

#include <winpr/crt.h>
#include <winpr/stream.h>

#define XRDP_SERVER_BEGIN_UPDATE		1
#define XRDP_SERVER_END_UPDATE			2
#define XRDP_SERVER_OPAQUE_RECT			3
#define XRDP_SERVER_SCREEN_BLT			4
#define XRDP_SERVER_PAINT_RECT			5
#define XRDP_SERVER_SET_CLIP			10
#define XRDP_SERVER_RESET_CLIP			11
#define XRDP_SERVER_SET_FORECOLOR		12
#define XRDP_SERVER_SET_BACKCOLOR		13
#define XRDP_SERVER_SET_ROP2			14
#define XRDP_SERVER_SET_PEN			17
#define XRDP_SERVER_LINE_TO			18
#define XRDP_SERVER_CREATE_OS_SURFACE		20
#define XRDP_SERVER_SWITCH_OS_SURFACE		21
#define XRDP_SERVER_DELETE_OS_SURFACE		22
#define XRDP_SERVER_MEMBLT			23
#define XRDP_SERVER_SET_HINTS			24
#define XRDP_SERVER_WINDOW_NEW_UPDATE		25
#define XRDP_SERVER_WINDOW_DELETE		26
#define XRDP_SERVER_SET_POINTER			51
#define XRDP_SERVER_SET_POINTER_EX		52
#define XRDP_SERVER_CREATE_FRAMEBUFFER		101

#define DEFINE_MSG_COMMON() \
	UINT32 type; \
	UINT32 length

struct _XRDP_MSG_COMMON
{
	DEFINE_MSG_COMMON();
};
typedef struct _XRDP_MSG_COMMON XRDP_MSG_COMMON;

struct _XRDP_MSG_BEGIN_UPDATE
{
	DEFINE_MSG_COMMON();
};
typedef struct _XRDP_MSG_BEGIN_UPDATE XRDP_MSG_BEGIN_UPDATE;

struct _XRDP_MSG_END_UPDATE
{
	DEFINE_MSG_COMMON();
};
typedef struct _XRDP_MSG_END_UPDATE XRDP_MSG_END_UPDATE;

struct _XRDP_MSG_OPAQUE_RECT
{
	DEFINE_MSG_COMMON();

	INT32 nLeftRect;
	INT32 nTopRect;
	INT32 nWidth;
	INT32 nHeight;
	UINT32 color;
};
typedef struct _XRDP_MSG_OPAQUE_RECT XRDP_MSG_OPAQUE_RECT;

struct _XRDP_MSG_SCREEN_BLT
{
	DEFINE_MSG_COMMON();

	INT32 nLeftRect;
	INT32 nTopRect;
	INT32 nWidth;
	INT32 nHeight;
	UINT32 bRop;
	INT32 nXSrc;
	INT32 nYSrc;
};
typedef struct _XRDP_MSG_SCREEN_BLT XRDP_MSG_SCREEN_BLT;

struct _XRDP_MSG_PAINT_RECT
{
	DEFINE_MSG_COMMON();

	INT32 nLeftRect;
	INT32 nTopRect;
	INT32 nWidth;
	INT32 nHeight;
	INT32 nXSrc;
	INT32 nYSrc;
	BYTE* bitmapData;
	UINT32 bitmapDataLength;
};
typedef struct _XRDP_MSG_PAINT_RECT XRDP_MSG_PAINT_RECT;

struct _XRDP_MSG_SET_CLIP
{
	DEFINE_MSG_COMMON();

	int x;
	int y;
	int width;
	int height;
};
typedef struct _XRDP_MSG_SET_CLIP XRDP_MSG_SET_CLIP;

struct _XRDP_MSG_RESET_CLIP
{
	DEFINE_MSG_COMMON();
};
typedef struct _XRDP_MSG_RESET_CLIP XRDP_MSG_RESET_CLIP;

struct _XRDP_MSG_SET_FORECOLOR
{
	DEFINE_MSG_COMMON();

	UINT32 ForeColor;
};
typedef struct _XRDP_MSG_SET_FORECOLOR XRDP_MSG_SET_FORECOLOR;

struct _XRDP_MSG_SET_BACKCOLOR
{
	DEFINE_MSG_COMMON();

	UINT32 BackColor;
};
typedef struct _XRDP_MSG_SET_BACKCOLOR XRDP_MSG_SET_BACKCOLOR;

struct _XRDP_MSG_SET_ROP2
{
	DEFINE_MSG_COMMON();

	UINT32 bRop2;
};
typedef struct _XRDP_MSG_SET_ROP2 XRDP_MSG_SET_ROP2;

struct _XRDP_MSG_SET_PEN
{
	DEFINE_MSG_COMMON();

	UINT32 PenStyle;
	UINT32 PenWidth;
	UINT32 PenColor;
};
typedef struct _XRDP_MSG_SET_PEN XRDP_MSG_SET_PEN;

struct _XRDP_MSG_LINE_TO
{
	DEFINE_MSG_COMMON();

	UINT32 backMode;
	INT32 nXStart;
	INT32 nYStart;
	INT32 nXEnd;
	INT32 nYEnd;
	UINT32 backColor;
	UINT32 bRop2;
	UINT32 penStyle;
	UINT32 penWidth;
	UINT32 penColor;
};
typedef struct _XRDP_MSG_LINE_TO XRDP_MSG_LINE_TO;

struct _XRDP_MSG_SET_POINTER
{
	DEFINE_MSG_COMMON();

	UINT32 xPos;
	UINT32 yPos;
	UINT32 width;
	UINT32 height;
	UINT32 lengthAndMask;
	UINT32 lengthXorMask;
	BYTE* xorMaskData;
	BYTE* andMaskData;
};
typedef struct _XRDP_MSG_SET_POINTER XRDP_MSG_SET_POINTER;

struct _XRDP_MSG_SET_POINTER_EX
{
	DEFINE_MSG_COMMON();

	UINT32 xorBpp;
	UINT32 xPos;
	UINT32 yPos;
	UINT32 width;
	UINT32 height;
	UINT32 lengthAndMask;
	UINT32 lengthXorMask;
	BYTE* xorMaskData;
	BYTE* andMaskData;
};
typedef struct _XRDP_MSG_SET_POINTER_EX XRDP_MSG_SET_POINTER_EX;

struct _XRDP_MSG_CREATE_OS_SURFACE
{
	DEFINE_MSG_COMMON();

	int index;
	UINT32 width;
	UINT32 height;
};
typedef struct _XRDP_MSG_CREATE_OS_SURFACE XRDP_MSG_CREATE_OS_SURFACE;

struct _XRDP_MSG_SWITCH_OS_SURFACE
{
	DEFINE_MSG_COMMON();

	int index;
};
typedef struct _XRDP_MSG_SWITCH_OS_SURFACE XRDP_MSG_SWITCH_OS_SURFACE;

struct _XRDP_MSG_DELETE_OS_SURFACE
{
	DEFINE_MSG_COMMON();

	int index;
};
typedef struct _XRDP_MSG_DELETE_OS_SURFACE XRDP_MSG_DELETE_OS_SURFACE;

struct _XRDP_MSG_MEMBLT
{
	DEFINE_MSG_COMMON();

	INT32 nLeftRect;
	INT32 nTopRect;
	INT32 nWidth;
	INT32 nHeight;
	UINT32 bRop;
	INT32 nXSrc;
	INT32 nYSrc;
	int index;
};
typedef struct _XRDP_MSG_MEMBLT XRDP_MSG_MEMBLT;

struct _XRDP_MSG_SET_HINTS
{
	DEFINE_MSG_COMMON();

	int hints;
	int mask;
};
typedef struct _XRDP_MSG_SET_HINTS XRDP_MSG_SET_HINTS;

struct _XRDP_MSG_WINDOW_NEW_UPDATE
{
	DEFINE_MSG_COMMON();

	UINT32 windowId;
	UINT32 ownerWindowId;
	UINT32 style;
	UINT32 extendedStyle;
	UINT32 showState;
	RAIL_UNICODE_STRING titleInfo;
	UINT32 clientOffsetX;
	UINT32 clientOffsetY;
	UINT32 clientAreaWidth;
	UINT32 clientAreaHeight;
	UINT32 RPContent;
	UINT32 rootParentHandle;
	UINT32 windowOffsetX;
	UINT32 windowOffsetY;
	UINT32 windowClientDeltaX;
	UINT32 windowClientDeltaY;
	UINT32 windowWidth;
	UINT32 windowHeight;
	UINT32 numWindowRects;
	RECTANGLE_16* windowRects;
	UINT32 visibleOffsetX;
	UINT32 visibleOffsetY;
	UINT32 numVisibilityRects;
	RECTANGLE_16* visibilityRects;
};
typedef struct _XRDP_MSG_WINDOW_NEW_UPDATE XRDP_MSG_WINDOW_NEW_UPDATE;

struct _XRDP_MSG_WINDOW_DELETE
{
	DEFINE_MSG_COMMON();

	UINT32 windowId;
};
typedef struct _XRDP_MSG_WINDOW_DELETE XRDP_MSG_WINDOW_DELETE;

struct _XRDP_MSG_CREATE_FRAMEBUFFER
{
	DEFINE_MSG_COMMON();

	int width;
	int height;
	int scanline;
	int segmentId;
	int bitsPerPixel;
	int bytesPerPixel;
};
typedef struct _XRDP_MSG_CREATE_FRAMEBUFFER XRDP_MSG_CREATE_FRAMEBUFFER;

#ifdef __cplusplus
extern "C" {
#endif

FREERDP_API int xrdp_prepare_msg(wStream* s, XRDP_MSG_COMMON* msg);

FREERDP_API int xrdp_write_begin_update(wStream* s, XRDP_MSG_BEGIN_UPDATE* msg);
FREERDP_API int xrdp_write_end_update(wStream* s, XRDP_MSG_END_UPDATE* msg);
FREERDP_API int xrdp_write_opaque_rect(wStream* s, XRDP_MSG_OPAQUE_RECT* msg);
FREERDP_API int xrdp_write_screen_blt(wStream* s, XRDP_MSG_SCREEN_BLT* msg);
FREERDP_API int xrdp_write_paint_rect(wStream* s, XRDP_MSG_PAINT_RECT* msg);
FREERDP_API int xrdp_write_set_clip(wStream* s, XRDP_MSG_SET_CLIP* msg);
FREERDP_API int xrdp_write_reset_clip(wStream* s, XRDP_MSG_RESET_CLIP* msg);
FREERDP_API int xrdp_write_set_forecolor(wStream* s, XRDP_MSG_SET_FORECOLOR* msg);
FREERDP_API int xrdp_write_set_backcolor(wStream* s, XRDP_MSG_SET_BACKCOLOR* msg);
FREERDP_API int xrdp_write_set_rop2(wStream* s, XRDP_MSG_SET_ROP2* msg);
FREERDP_API int xrdp_write_set_pen(wStream* s, XRDP_MSG_SET_PEN* msg);
FREERDP_API int xrdp_write_line_to(wStream* s, XRDP_MSG_LINE_TO* msg);
FREERDP_API int xrdp_write_set_pointer(wStream* s, XRDP_MSG_SET_POINTER* msg);
FREERDP_API int xrdp_write_set_pointer_ex(wStream* s, XRDP_MSG_SET_POINTER_EX* msg);
FREERDP_API int xrdp_write_create_os_surface(wStream* s, XRDP_MSG_CREATE_OS_SURFACE* msg);
FREERDP_API int xrdp_write_switch_os_surface(wStream* s, XRDP_MSG_SWITCH_OS_SURFACE* msg);
FREERDP_API int xrdp_write_delete_os_surface(wStream* s, XRDP_MSG_DELETE_OS_SURFACE* msg);
FREERDP_API int xrdp_write_memblt(wStream* s, XRDP_MSG_MEMBLT* msg);
FREERDP_API int xrdp_write_set_hints(wStream* s, XRDP_MSG_SET_HINTS* msg);
FREERDP_API int xrdp_write_window_new_update(wStream* s, XRDP_MSG_WINDOW_NEW_UPDATE* msg);
FREERDP_API int xrdp_write_window_delete(wStream* s, XRDP_MSG_WINDOW_DELETE* msg);
FREERDP_API int xrdp_write_create_framebuffer(wStream* s, XRDP_MSG_CREATE_FRAMEBUFFER* msg);

#ifdef __cplusplus
}
#endif

#endif /* XRDP_NG_H */
