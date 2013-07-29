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

#include <freerdp/gdi/gdi.h>

#include <winpr/crt.h>
#include <winpr/stream.h>

/* Common Data Types */

#define XRDP_MSG_FLAG_RECT		0x00000001

struct _XRDP_RECT
{
	UINT32 left;
	UINT32 top;
	UINT32 right;
	UINT32 bottom;
};
typedef struct _XRDP_RECT XRDP_RECT;

#define DEFINE_MSG_COMMON() \
	UINT32 type; \
	UINT32 length; \
	UINT32 msgFlags; \
	XRDP_RECT rect

struct _XRDP_MSG_COMMON
{
	DEFINE_MSG_COMMON();
};
typedef struct _XRDP_MSG_COMMON XRDP_MSG_COMMON;

struct _XRDP_FRAMEBUFFER
{
	int fbWidth;
	int fbHeight;
	int fbAttached;
	int fbScanline;
	int fbSegmentId;
	int fbBitsPerPixel;
	int fbBytesPerPixel;
	BYTE* fbSharedMemory;
};
typedef struct _XRDP_FRAMEBUFFER XRDP_FRAMEBUFFER;

#define XRDP_CODEC_JPEG			0x00000001
#define XRDP_CODEC_NSCODEC		0x00000002
#define XRDP_CODEC_REMOTEFX		0x00000004

int xrdp_read_common_header(wStream* s, XRDP_MSG_COMMON* msg);
int xrdp_write_common_header(wStream* s, XRDP_MSG_COMMON* msg);

/* Client Message Types */

#define XRDP_CLIENT_EVENT			101
#define XRDP_CLIENT_CAPABILITIES		102
#define XRDP_CLIENT_REFRESH_RECT		103

struct _XRDP_MSG_EVENT
{
	DEFINE_MSG_COMMON();

	UINT32 subType;
	UINT32 param1;
	UINT32 param2;
	UINT32 param3;
	UINT32 param4;
};
typedef struct _XRDP_MSG_EVENT XRDP_MSG_EVENT;

struct _XRDP_MSG_CAPABILITIES
{
	DEFINE_MSG_COMMON();

	UINT32 DesktopWidth;
	UINT32 DesktopHeight;
	UINT32 ColorDepth;
	UINT32 SupportedCodecs;
	UINT32 OffscreenSupportLevel;
	UINT32 OffscreenCacheSize;
	UINT32 OffscreenCacheEntries;
	UINT32 RailSupportLevel;
	UINT32 PointerFlags;
};
typedef struct _XRDP_MSG_CAPABILITIES XRDP_MSG_CAPABILITIES;

struct _XRDP_MSG_REFRESH_RECT
{
	DEFINE_MSG_COMMON();

	UINT32 numberOfAreas;
	RECTANGLE_16* areasToRefresh;
};
typedef struct _XRDP_MSG_REFRESH_RECT XRDP_MSG_REFRESH_RECT;

int xrdp_read_event(wStream* s, XRDP_MSG_EVENT* msg);
int xrdp_write_event(wStream* s, XRDP_MSG_EVENT* msg);

int xrdp_read_capabilities(wStream* s, XRDP_MSG_CAPABILITIES* msg);
int xrdp_write_capabilities(wStream* s, XRDP_MSG_CAPABILITIES* msg);

int xrdp_read_refresh_rect(wStream* s, XRDP_MSG_REFRESH_RECT* msg);
int xrdp_write_refresh_rect(wStream* s, XRDP_MSG_REFRESH_RECT* msg);

/* Server Message Types */

#define XRDP_SERVER_BEGIN_UPDATE		1
#define XRDP_SERVER_END_UPDATE			2
#define XRDP_SERVER_OPAQUE_RECT			3
#define XRDP_SERVER_SCREEN_BLT			4
#define XRDP_SERVER_PAINT_RECT			5
#define XRDP_SERVER_BEEP			6
#define XRDP_SERVER_MESSAGE			7
#define XRDP_SERVER_IS_TERMINATED		8
#define XRDP_SERVER_PATBLT			9
#define XRDP_SERVER_DSTBLT			10
#define XRDP_SERVER_SET_CLIPPING_REGION		12
#define XRDP_SERVER_SET_PALETTE			13
#define XRDP_SERVER_SET_ROP2			14
#define XRDP_SERVER_LINE_TO			18
#define XRDP_SERVER_SET_NULL_CLIPPING_REGION	19
#define XRDP_SERVER_CREATE_OFFSCREEN_SURFACE	20
#define XRDP_SERVER_SWITCH_OFFSCREEN_SURFACE	21
#define XRDP_SERVER_DELETE_OFFSCREEN_SURFACE	22
#define XRDP_SERVER_MEMBLT			23
#define XRDP_SERVER_CACHE_GLYPH			24
#define XRDP_SERVER_GLYPH_INDEX			25
#define XRDP_SERVER_RESET			29
#define XRDP_SERVER_WINDOW_NEW_UPDATE		30
#define XRDP_SERVER_WINDOW_DELETE		31
#define XRDP_SERVER_SET_POINTER			51
#define XRDP_SERVER_SET_POINTER_EX		52
#define XRDP_SERVER_SHARED_FRAMEBUFFER		101

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
	UINT32 fbSegmentId;
	XRDP_FRAMEBUFFER* framebuffer;
};
typedef struct _XRDP_MSG_PAINT_RECT XRDP_MSG_PAINT_RECT;

struct _XRDP_MSG_DSTBLT
{
	DEFINE_MSG_COMMON();

	INT32 nLeftRect;
	INT32 nTopRect;
	INT32 nWidth;
	INT32 nHeight;
	UINT32 bRop;
};
typedef struct _XRDP_MSG_DSTBLT XRDP_MSG_DSTBLT;

struct _XRDP_MSG_PATBLT
{
	DEFINE_MSG_COMMON();

	INT32 nLeftRect;
	INT32 nTopRect;
	INT32 nWidth;
	INT32 nHeight;
	UINT32 bRop;
	UINT32 backColor;
	UINT32 foreColor;
	rdpBrush brush;
};
typedef struct _XRDP_MSG_PATBLT XRDP_MSG_PATBLT;

struct _XRDP_MSG_SET_CLIPPING_REGION
{
	DEFINE_MSG_COMMON();

	BOOL bNullRegion;
	INT32 nLeftRect;
	INT32 nTopRect;
	INT32 nWidth;
	INT32 nHeight;
};
typedef struct _XRDP_MSG_SET_CLIPPING_REGION XRDP_MSG_SET_CLIPPING_REGION;

struct _XRDP_MSG_SET_ROP2
{
	DEFINE_MSG_COMMON();

	UINT32 bRop2;
};
typedef struct _XRDP_MSG_SET_ROP2 XRDP_MSG_SET_ROP2;

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
typedef struct _XRDP_MSG_SET_POINTER XRDP_MSG_SET_POINTER;

struct _XRDP_MSG_SET_PALETTE
{
	DEFINE_MSG_COMMON();

	UINT32 palette[256];
};
typedef struct _XRDP_MSG_SET_PALETTE XRDP_MSG_SET_PALETTE;

struct _XRDP_MSG_CREATE_OFFSCREEN_SURFACE
{
	DEFINE_MSG_COMMON();

	UINT32 cacheIndex;
	UINT32 nWidth;
	UINT32 nHeight;
};
typedef struct _XRDP_MSG_CREATE_OFFSCREEN_SURFACE XRDP_MSG_CREATE_OFFSCREEN_SURFACE;

struct _XRDP_MSG_SWITCH_OFFSCREEN_SURFACE
{
	DEFINE_MSG_COMMON();

	UINT32 cacheIndex;
};
typedef struct _XRDP_MSG_SWITCH_OFFSCREEN_SURFACE XRDP_MSG_SWITCH_OFFSCREEN_SURFACE;

struct _XRDP_MSG_DELETE_OFFSCREEN_SURFACE
{
	DEFINE_MSG_COMMON();

	UINT32 cacheIndex;
};
typedef struct _XRDP_MSG_DELETE_OFFSCREEN_SURFACE XRDP_MSG_DELETE_OFFSCREEN_SURFACE;

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

struct _XRDP_MSG_CACHE_GLYPH
{
	DEFINE_MSG_COMMON();

	UINT32 cacheId;
	UINT32 flags;
	UINT32 cGlyphs;
	GLYPH_DATA_V2 glyphData[256];
	BYTE* unicodeCharacters;
};
typedef struct _XRDP_MSG_CACHE_GLYPH XRDP_MSG_CACHE_GLYPH;

struct _XRDP_MSG_GLYPH_INDEX
{
	DEFINE_MSG_COMMON();

	UINT32 cacheId;
	UINT32 flAccel;
	UINT32 ulCharInc;
	UINT32 fOpRedundant;
	UINT32 backColor;
	UINT32 foreColor;
	INT32 bkLeft;
	INT32 bkTop;
	INT32 bkRight;
	INT32 bkBottom;
	INT32 opLeft;
	INT32 opTop;
	INT32 opRight;
	INT32 opBottom;
	rdpBrush brush;
	INT32 x;
	INT32 y;
	UINT32 cbData;
	BYTE data[256];
};
typedef struct _XRDP_MSG_GLYPH_INDEX XRDP_MSG_GLYPH_INDEX;

struct _XRDP_MSG_RESET
{
	DEFINE_MSG_COMMON();

	UINT32 DesktopWidth;
	UINT32 DesktopHeight;
	UINT32 ColorDepth;
};
typedef struct _XRDP_MSG_RESET XRDP_MSG_RESET;

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

struct _XRDP_MSG_SHARED_FRAMEBUFFER
{
	DEFINE_MSG_COMMON();

	int width;
	int height;
	int attach;
	int scanline;
	int segmentId;
	int bitsPerPixel;
	int bytesPerPixel;
};
typedef struct _XRDP_MSG_SHARED_FRAMEBUFFER XRDP_MSG_SHARED_FRAMEBUFFER;

#ifdef __cplusplus
extern "C" {
#endif

FREERDP_API int xrdp_prepare_msg(wStream* s, XRDP_MSG_COMMON* msg);
FREERDP_API char* xrdp_get_msg_type_string(UINT32 type);

FREERDP_API int xrdp_read_begin_update(wStream* s, XRDP_MSG_BEGIN_UPDATE* msg);
FREERDP_API int xrdp_write_begin_update(wStream* s, XRDP_MSG_BEGIN_UPDATE* msg);

FREERDP_API int xrdp_read_end_update(wStream* s, XRDP_MSG_END_UPDATE* msg);
FREERDP_API int xrdp_write_end_update(wStream* s, XRDP_MSG_END_UPDATE* msg);

FREERDP_API int xrdp_read_opaque_rect(wStream* s, XRDP_MSG_OPAQUE_RECT* msg);
FREERDP_API int xrdp_write_opaque_rect(wStream* s, XRDP_MSG_OPAQUE_RECT* msg);

FREERDP_API int xrdp_read_screen_blt(wStream* s, XRDP_MSG_SCREEN_BLT* msg);
FREERDP_API int xrdp_write_screen_blt(wStream* s, XRDP_MSG_SCREEN_BLT* msg);

FREERDP_API int xrdp_read_patblt(wStream* s, XRDP_MSG_PATBLT* msg);
FREERDP_API int xrdp_write_patblt(wStream* s, XRDP_MSG_PATBLT* msg);

FREERDP_API int xrdp_read_dstblt(wStream* s, XRDP_MSG_DSTBLT* msg);
FREERDP_API int xrdp_write_dstblt(wStream* s, XRDP_MSG_DSTBLT* msg);

FREERDP_API int xrdp_read_paint_rect(wStream* s, XRDP_MSG_PAINT_RECT* msg);
FREERDP_API int xrdp_write_paint_rect(wStream* s, XRDP_MSG_PAINT_RECT* msg);

FREERDP_API int xrdp_read_set_clipping_region(wStream* s, XRDP_MSG_SET_CLIPPING_REGION* msg);
FREERDP_API int xrdp_write_set_clipping_region(wStream* s, XRDP_MSG_SET_CLIPPING_REGION* msg);

FREERDP_API int xrdp_read_set_rop2(wStream* s, XRDP_MSG_SET_ROP2* msg);
FREERDP_API int xrdp_write_set_rop2(wStream* s, XRDP_MSG_SET_ROP2* msg);

FREERDP_API int xrdp_read_line_to(wStream* s, XRDP_MSG_LINE_TO* msg);
FREERDP_API int xrdp_write_line_to(wStream* s, XRDP_MSG_LINE_TO* msg);

FREERDP_API int xrdp_read_set_pointer(wStream* s, XRDP_MSG_SET_POINTER* msg);
FREERDP_API int xrdp_write_set_pointer(wStream* s, XRDP_MSG_SET_POINTER* msg);

FREERDP_API int xrdp_read_create_offscreen_surface(wStream* s, XRDP_MSG_CREATE_OFFSCREEN_SURFACE* msg);
FREERDP_API int xrdp_write_create_offscreen_surface(wStream* s, XRDP_MSG_CREATE_OFFSCREEN_SURFACE* msg);

FREERDP_API int xrdp_read_switch_offscreen_surface(wStream* s, XRDP_MSG_SWITCH_OFFSCREEN_SURFACE* msg);
FREERDP_API int xrdp_write_switch_offscreen_surface(wStream* s, XRDP_MSG_SWITCH_OFFSCREEN_SURFACE* msg);

FREERDP_API int xrdp_read_delete_offscreen_surface(wStream* s, XRDP_MSG_DELETE_OFFSCREEN_SURFACE* msg);
FREERDP_API int xrdp_write_delete_offscreen_surface(wStream* s, XRDP_MSG_DELETE_OFFSCREEN_SURFACE* msg);

FREERDP_API int xrdp_read_memblt(wStream* s, XRDP_MSG_MEMBLT* msg);
FREERDP_API int xrdp_write_memblt(wStream* s, XRDP_MSG_MEMBLT* msg);

FREERDP_API int xrdp_read_window_new_update(wStream* s, XRDP_MSG_WINDOW_NEW_UPDATE* msg);
FREERDP_API int xrdp_write_window_new_update(wStream* s, XRDP_MSG_WINDOW_NEW_UPDATE* msg);

FREERDP_API int xrdp_read_window_delete(wStream* s, XRDP_MSG_WINDOW_DELETE* msg);
FREERDP_API int xrdp_write_window_delete(wStream* s, XRDP_MSG_WINDOW_DELETE* msg);

FREERDP_API int xrdp_read_shared_framebuffer(wStream* s, XRDP_MSG_SHARED_FRAMEBUFFER* msg);
FREERDP_API int xrdp_write_shared_framebuffer(wStream* s, XRDP_MSG_SHARED_FRAMEBUFFER* msg);

#ifdef __cplusplus
}
#endif

#endif /* XRDP_NG_H */
