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
#include <winpr/synch.h>
#include <winpr/stream.h>

typedef struct xrdp_session xrdpSession;

/* Common Data Types */

#define XRDP_MSG_FLAG_RECT		0x00000001

/**
 * XRDP_RECT matches the memory layout of pixman_rectangle32_t:
 *
 * struct pixman_rectangle32
 * {
 * 	int32_t x, y;
 * 	uint32_t width, height;
 * };
 */

struct _XRDP_RECT
{
	INT32 x;
	INT32 y;
	UINT32 width;
	UINT32 height;
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
	void* image;
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
#define XRDP_CLIENT_SYNCHRONIZE_KEYBOARD_EVENT	104
#define XRDP_CLIENT_SCANCODE_KEYBOARD_EVENT	105
#define XRDP_CLIENT_VIRTUAL_KEYBOARD_EVENT	106
#define XRDP_CLIENT_UNICODE_KEYBOARD_EVENT	107
#define XRDP_CLIENT_MOUSE_EVENT			108
#define XRDP_CLIENT_EXTENDED_MOUSE_EVENT	109

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

struct _XRDP_MSG_SYNCHRONIZE_KEYBOARD_EVENT
{
	DEFINE_MSG_COMMON();

	UINT32 flags;
};
typedef struct _XRDP_MSG_SYNCHRONIZE_KEYBOARD_EVENT XRDP_MSG_SYNCHRONIZE_KEYBOARD_EVENT;

struct _XRDP_MSG_SCANCODE_KEYBOARD_EVENT
{
	DEFINE_MSG_COMMON();

	UINT32 flags;
	UINT32 code;
	UINT32 keyboardType;
};
typedef struct _XRDP_MSG_SCANCODE_KEYBOARD_EVENT XRDP_MSG_SCANCODE_KEYBOARD_EVENT;

struct _XRDP_MSG_VIRTUAL_KEYBOARD_EVENT
{
	DEFINE_MSG_COMMON();

	UINT32 flags;
	UINT32 code;
};
typedef struct _XRDP_MSG_VIRTUAL_KEYBOARD_EVENT XRDP_MSG_VIRTUAL_KEYBOARD_EVENT;

struct _XRDP_MSG_UNICODE_KEYBOARD_EVENT
{
	DEFINE_MSG_COMMON();

	UINT32 flags;
	UINT32 code;
};
typedef struct _XRDP_MSG_UNICODE_KEYBOARD_EVENT XRDP_MSG_UNICODE_KEYBOARD_EVENT;

struct _XRDP_MSG_MOUSE_EVENT
{
	DEFINE_MSG_COMMON();

	DWORD flags;
	DWORD x;
	DWORD y;
};
typedef struct _XRDP_MSG_MOUSE_EVENT XRDP_MSG_MOUSE_EVENT;

struct _XRDP_MSG_EXTENDED_MOUSE_EVENT
{
	DEFINE_MSG_COMMON();

	DWORD flags;
	DWORD x;
	DWORD y;
};
typedef struct _XRDP_MSG_EXTENDED_MOUSE_EVENT XRDP_MSG_EXTENDED_MOUSE_EVENT;

struct _XRDP_MSG_CAPABILITIES
{
	DEFINE_MSG_COMMON();

	UINT32 DesktopWidth;
	UINT32 DesktopHeight;
	UINT32 ColorDepth;
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

int xrdp_read_synchronize_keyboard_event(wStream* s, XRDP_MSG_SYNCHRONIZE_KEYBOARD_EVENT* msg);
int xrdp_write_synchronize_keyboard_event(wStream* s, XRDP_MSG_SYNCHRONIZE_KEYBOARD_EVENT* msg);

int xrdp_read_virtual_keyboard_event(wStream* s, XRDP_MSG_VIRTUAL_KEYBOARD_EVENT* msg);
int xrdp_write_virtual_keyboard_event(wStream* s, XRDP_MSG_VIRTUAL_KEYBOARD_EVENT* msg);

int xrdp_read_scancode_keyboard_event(wStream* s, XRDP_MSG_SCANCODE_KEYBOARD_EVENT* msg);
int xrdp_write_scancode_keyboard_event(wStream* s, XRDP_MSG_SCANCODE_KEYBOARD_EVENT* msg);

int xrdp_read_unicode_keyboard_event(wStream* s, XRDP_MSG_UNICODE_KEYBOARD_EVENT* msg);
int xrdp_write_unicode_keyboard_event(wStream* s, XRDP_MSG_UNICODE_KEYBOARD_EVENT* msg);

int xrdp_read_mouse_event(wStream* s, XRDP_MSG_MOUSE_EVENT* msg);
int xrdp_write_mouse_event(wStream* s, XRDP_MSG_MOUSE_EVENT* msg);

int xrdp_read_extended_mouse_event(wStream* s, XRDP_MSG_EXTENDED_MOUSE_EVENT* msg);
int xrdp_write_extended_mouse_event(wStream* s, XRDP_MSG_EXTENDED_MOUSE_EVENT* msg);

int xrdp_read_capabilities(wStream* s, XRDP_MSG_CAPABILITIES* msg);
int xrdp_write_capabilities(wStream* s, XRDP_MSG_CAPABILITIES* msg);

int xrdp_read_refresh_rect(wStream* s, XRDP_MSG_REFRESH_RECT* msg);
int xrdp_write_refresh_rect(wStream* s, XRDP_MSG_REFRESH_RECT* msg);

/* Server Message Types */

#define XRDP_SERVER_BEGIN_UPDATE		1
#define XRDP_SERVER_END_UPDATE			2
#define XRDP_SERVER_SET_CLIPPING_REGION		3
#define XRDP_SERVER_OPAQUE_RECT			4
#define XRDP_SERVER_SCREEN_BLT			5
#define XRDP_SERVER_PAINT_RECT			6
#define XRDP_SERVER_PATBLT			7
#define XRDP_SERVER_DSTBLT			8
#define XRDP_SERVER_LINE_TO			9
#define XRDP_SERVER_CREATE_OFFSCREEN_SURFACE	10
#define XRDP_SERVER_SWITCH_OFFSCREEN_SURFACE	11
#define XRDP_SERVER_DELETE_OFFSCREEN_SURFACE	12
#define XRDP_SERVER_PAINT_OFFSCREEN_SURFACE	13
#define XRDP_SERVER_SET_PALETTE			14
#define XRDP_SERVER_CACHE_GLYPH			15
#define XRDP_SERVER_GLYPH_INDEX			16
#define XRDP_SERVER_SET_POINTER			17
#define XRDP_SERVER_SHARED_FRAMEBUFFER		18
#define XRDP_SERVER_BEEP			19
#define XRDP_SERVER_RESET			20
#define XRDP_SERVER_WINDOW_NEW_UPDATE		21
#define XRDP_SERVER_WINDOW_DELETE		22

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

	UINT32* palette;
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

struct _XRDP_MSG_PAINT_OFFSCREEN_SURFACE
{
	DEFINE_MSG_COMMON();

	UINT32 cacheIndex;
	INT32 nLeftRect;
	INT32 nTopRect;
	INT32 nWidth;
	INT32 nHeight;
	UINT32 bRop;
	INT32 nXSrc;
	INT32 nYSrc;
};
typedef struct _XRDP_MSG_PAINT_OFFSCREEN_SURFACE XRDP_MSG_PAINT_OFFSCREEN_SURFACE;

struct _XRDP_GLYPH_DATA
{
	UINT32 cacheIndex;
	INT32 x;
	INT32 y;
	UINT32 cx;
	UINT32 cy;
	UINT32 cb;
	BYTE* aj;
};
typedef struct _XRDP_GLYPH_DATA XRDP_GLYPH_DATA;

struct _XRDP_MSG_CACHE_GLYPH
{
	DEFINE_MSG_COMMON();

	UINT32 cacheId;
	UINT32 flags;
	UINT32 cGlyphs;
	XRDP_GLYPH_DATA* glyphData;
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
	BYTE* data;
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

struct _XRDP_MSG_BEEP
{
	DEFINE_MSG_COMMON();

};
typedef struct _XRDP_MSG_BEEP XRDP_MSG_BEEP;

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

union _XRDP_MSG_SERVER
{
	XRDP_MSG_BEGIN_UPDATE BeginUpdate;
	XRDP_MSG_END_UPDATE EndUpdate;
	XRDP_MSG_SET_CLIPPING_REGION SetClippingRegion;
	XRDP_MSG_OPAQUE_RECT OpaqueRect;
	XRDP_MSG_SCREEN_BLT ScreenBlt;
	XRDP_MSG_PAINT_RECT PaintRect;
	XRDP_MSG_PATBLT PatBlt;
	XRDP_MSG_DSTBLT DstBlt;
	XRDP_MSG_LINE_TO LineTo;
	XRDP_MSG_CREATE_OFFSCREEN_SURFACE CreateOffscreenSurface;
	XRDP_MSG_SWITCH_OFFSCREEN_SURFACE SwitchOffscreenSurface;
	XRDP_MSG_DELETE_OFFSCREEN_SURFACE DeleteOffscreenSurface;
	XRDP_MSG_PAINT_OFFSCREEN_SURFACE PaintOffscreenSurface;
	XRDP_MSG_SET_PALETTE SetPalette;
	XRDP_MSG_CACHE_GLYPH CacheGlyph;
	XRDP_MSG_GLYPH_INDEX GlyphIndex;
	XRDP_MSG_SET_POINTER SetPointer;
	XRDP_MSG_SHARED_FRAMEBUFFER SharedFramebuffer;
	XRDP_MSG_BEEP Beep;
	XRDP_MSG_RESET Reset;
	XRDP_MSG_WINDOW_NEW_UPDATE WindowNewUpdate;
	XRDP_MSG_WINDOW_DELETE WindowDelete;
};
typedef union _XRDP_MSG_SERVER XRDP_MSG_SERVER;

/**
 * Module Interface
 */

typedef struct xrdp_mod xrdpModule;

typedef int (*pXrdpModuleStart)(xrdpModule* mod);
typedef int (*pXrdpModuleStop)(xrdpModule* mod);
typedef int (*pXrdpModuleConnect)(xrdpModule* mod);
typedef int (*pXrdpModuleGetEventHandles)(xrdpModule* mod, HANDLE* events, DWORD* nCount);
typedef int (*pXrdpModuleCheckEventHandles)(xrdpModule* mod);

typedef int (*pXrdpClientSynchronizeKeyboardEvent)(xrdpModule* mod, DWORD flags);
typedef int (*pXrdpClientScancodeKeyboardEvent)(xrdpModule* mod, DWORD flags, DWORD code, DWORD keyboardType);
typedef int (*pXrdpClientVirtualKeyboardEvent)(xrdpModule* mod, DWORD flags, DWORD code);
typedef int (*pXrdpClientUnicodeKeyboardEvent)(xrdpModule* mod, DWORD flags, DWORD code);
typedef int (*pXrdpClientMouseEvent)(xrdpModule* mod, DWORD flags, DWORD x, DWORD y);
typedef int (*pXrdpClientExtendedMouseEvent)(xrdpModule* mod, DWORD flags, DWORD x, DWORD y);
typedef int (*pXrdpClientSessionChange)(xrdpModule* mod, int width, int height);

struct xrdp_client_module
{
	pXrdpClientSynchronizeKeyboardEvent SynchronizeKeyboardEvent;
	pXrdpClientScancodeKeyboardEvent ScancodeKeyboardEvent;
	pXrdpClientVirtualKeyboardEvent VirtualKeyboardEvent;
	pXrdpClientUnicodeKeyboardEvent UnicodeKeyboardEvent;
	pXrdpClientMouseEvent MouseEvent;
	pXrdpClientExtendedMouseEvent ExtendedMouseEvent;
	pXrdpClientSessionChange SessionChange;
};
typedef struct xrdp_client_module xrdpClientModule;

typedef int (*pXrdpServerIsTerminated)(xrdpModule* mod);

typedef int (*pXrdpServerBeginUpdate)(xrdpModule* mod, XRDP_MSG_BEGIN_UPDATE* msg);
typedef int (*pXrdpServerEndUpdate)(xrdpModule* mod, XRDP_MSG_END_UPDATE* msg);
typedef int (*pXrdpServerBeep)(xrdpModule* mod, XRDP_MSG_BEEP* msg);
typedef int (*pXrdpServerOpaqueRect)(xrdpModule* mod, XRDP_MSG_OPAQUE_RECT* msg);
typedef int (*pXrdpServerScreenBlt)(xrdpModule* mod, XRDP_MSG_SCREEN_BLT* msg);
typedef int (*pXrdpServerPaintRect)(xrdpModule* mod, XRDP_MSG_PAINT_RECT* msg);
typedef int (*pXrdpServerPatBlt)(xrdpModule* mod, XRDP_MSG_PATBLT* msg);
typedef int (*pXrdpServerDstBlt)(xrdpModule* mod, XRDP_MSG_DSTBLT* msg);
typedef int (*pXrdpServerSetPointer)(xrdpModule* mod, XRDP_MSG_SET_POINTER* msg);
typedef int (*pXrdpServerSetPalette)(xrdpModule* mod, XRDP_MSG_SET_PALETTE* msg);
typedef int (*pXrdpServerSetClippingRegion)(xrdpModule* mod, XRDP_MSG_SET_CLIPPING_REGION* msg);
typedef int (*pXrdpServerLineTo)(xrdpModule* mod, XRDP_MSG_LINE_TO* msg);
typedef int (*pXrdpServerCacheGlyph)(xrdpModule* mod, XRDP_MSG_CACHE_GLYPH* msg);
typedef int (*pXrdpServerGlyphIndex)(xrdpModule* mod, XRDP_MSG_GLYPH_INDEX* msg);
typedef int (*pXrdpServerSharedFramebuffer)(xrdpModule* mod, XRDP_MSG_SHARED_FRAMEBUFFER* msg);
typedef int (*pXrdpServerReset)(xrdpModule* mod, XRDP_MSG_RESET* msg);
typedef int (*pXrdpServerCreateOffscreenSurface)(xrdpModule* mod, XRDP_MSG_CREATE_OFFSCREEN_SURFACE* msg);
typedef int (*pXrdpServerSwitchOffscreenSurface)(xrdpModule* mod, XRDP_MSG_SWITCH_OFFSCREEN_SURFACE* msg);
typedef int (*pXrdpServerDeleteOffscreenSurface)(xrdpModule* mod, XRDP_MSG_DELETE_OFFSCREEN_SURFACE* msg);
typedef int (*pXrdpServerPaintOffscreenSurface)(xrdpModule* mod, XRDP_MSG_PAINT_OFFSCREEN_SURFACE* msg);

typedef int (*pXrdpServerWindowNewUpdate)(xrdpModule* mod, XRDP_MSG_WINDOW_NEW_UPDATE* msg);
typedef int (*pXrdpServerWindowDelete)(xrdpModule* mod, XRDP_MSG_WINDOW_DELETE* msg);

struct xrdp_server_module
{
	pXrdpServerBeginUpdate BeginUpdate;
	pXrdpServerEndUpdate EndUpdate;
	pXrdpServerBeep Beep;
	pXrdpServerIsTerminated IsTerminated;
	pXrdpServerOpaqueRect OpaqueRect;
	pXrdpServerScreenBlt ScreenBlt;
	pXrdpServerPaintRect PaintRect;
	pXrdpServerPatBlt PatBlt;
	pXrdpServerDstBlt DstBlt;
	pXrdpServerSetPointer SetPointer;
	pXrdpServerSetPalette SetPalette;
	pXrdpServerSetClippingRegion SetClippingRegion;
	pXrdpServerLineTo LineTo;
	pXrdpServerCacheGlyph CacheGlyph;
	pXrdpServerGlyphIndex GlyphIndex;
	pXrdpServerSharedFramebuffer SharedFramebuffer;
	pXrdpServerReset Reset;
	pXrdpServerCreateOffscreenSurface CreateOffscreenSurface;
	pXrdpServerSwitchOffscreenSurface SwitchOffscreenSurface;
	pXrdpServerDeleteOffscreenSurface DeleteOffscreenSurface;
	pXrdpServerPaintOffscreenSurface PaintOffscreenSurface;
	pXrdpServerWindowNewUpdate WindowNewUpdate;
	pXrdpServerWindowDelete WindowDelete;
};
typedef struct xrdp_server_module xrdpServerModule;

struct xrdp_mod
{
	int size;

	xrdpClientModule* client;
	xrdpServerModule* server;

	pXrdpModuleStart Start;
	pXrdpModuleStop Stop;
	pXrdpModuleConnect Connect;
	pXrdpModuleGetEventHandles GetEventHandles;
	pXrdpModuleCheckEventHandles CheckEventHandles;

	DWORD SessionId;
	xrdpSession* session;

	freerdp* instance;
	rdpSettings* settings;

	UINT32 TotalLength;
	UINT32 TotalCount;
	HANDLE SocketEvent;
	wStream* SendStream;
	wStream* ReceiveStream;

	XRDP_FRAMEBUFFER framebuffer;

	int fps;
	int MaxFps;
	HANDLE StopEvent;
	HANDLE ServerTimer;
	HANDLE ServerThread;
	wLinkedList* ServerList;
	wMessageQueue* ServerQueue;
	xrdpServerModule* ServerProxy;

	/* Clean Module Interface */

	HANDLE hClientPipe;
	HANDLE hServerPipe;
};

#ifdef __cplusplus
extern "C" {
#endif

FREERDP_API int xrdp_server_message_size(UINT32 type);
FREERDP_API char* xrdp_server_message_name(UINT32 type);

FREERDP_API int xrdp_server_message_read(wStream* s, XRDP_MSG_COMMON* msg);
FREERDP_API int xrdp_server_message_write(wStream* s, XRDP_MSG_COMMON* msg);

FREERDP_API void* xrdp_server_message_copy(XRDP_MSG_COMMON* msg);
FREERDP_API void xrdp_server_message_free(XRDP_MSG_COMMON* msg);

/**
 * New Clean Module Interface API
 */

FREERDP_API xrdpClientModule* freerds_client_outbound_interface_new();

FREERDP_API int freerds_named_pipe_read(HANDLE hNamedPipe, BYTE* data, DWORD length);
FREERDP_API int freerds_named_pipe_write(HANDLE hNamedPipe, BYTE* data, DWORD length);

#ifdef __cplusplus
}
#endif

#endif /* XRDP_NG_H */
