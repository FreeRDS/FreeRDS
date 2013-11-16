/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
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

#ifndef FREERDS_H
#define FREERDS_H

#include <winpr/crt.h>
#include <winpr/synch.h>
#include <winpr/stream.h>
#include <winpr/collections.h>

#include <freerdp/api.h>
#include <freerdp/freerdp.h>

#include <freerdp/gdi/gdi.h>

typedef struct rds_module_connector rdsModuleConnector;

typedef struct rds_connection rdsConnection;

/* Common Data Types */

#define RDS_MSG_FLAG_RECT		0x00000001

/**
 * RDS_RECT matches the memory layout of pixman_rectangle32_t:
 *
 * struct pixman_rectangle32
 * {
 * 	int32_t x, y;
 * 	uint32_t width, height;
 * };
 */

struct _RDS_RECT
{
	INT32 x;
	INT32 y;
	UINT32 width;
	UINT32 height;
};
typedef struct _RDS_RECT RDS_RECT;

#define DEFINE_MSG_COMMON() \
	UINT32 type; \
	UINT32 length; \
	UINT32 msgFlags; \
	RDS_RECT rect

struct _RDS_MSG_COMMON
{
	DEFINE_MSG_COMMON();
};
typedef struct _RDS_MSG_COMMON RDS_MSG_COMMON;

struct _RDS_FRAMEBUFFER
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
typedef struct _RDS_FRAMEBUFFER RDS_FRAMEBUFFER;

#define RDS_CODEC_JPEG			0x00000001
#define RDS_CODEC_NSCODEC		0x00000002
#define RDS_CODEC_REMOTEFX		0x00000004

struct _RDS_MSG_LOGON_USER
{
	DEFINE_MSG_COMMON();

	UINT32 Flags;
	UINT32 UserLength;
	UINT32 DomainLength;
	UINT32 PasswordLength;

	char* User;
	char* Domain;
	char* Password;
};
typedef struct _RDS_MSG_LOGON_USER RDS_MSG_LOGON_USER;

struct _RDS_MSG_LOGOFF_USER
{
	DEFINE_MSG_COMMON();

	UINT32 Flags;
};
typedef struct _RDS_MSG_LOGOFF_USER RDS_MSG_LOGOFF_USER;

#ifdef __cplusplus
extern "C" {
#endif

UINT32 freerds_peek_common_header_length(BYTE* data);

int freerds_read_common_header(wStream* s, RDS_MSG_COMMON* msg);
int freerds_write_common_header(wStream* s, RDS_MSG_COMMON* msg);

int freerds_read_logon_user(wStream* s, RDS_MSG_LOGON_USER* msg);
int freerds_write_logon_user(wStream* s, RDS_MSG_LOGON_USER* msg);

int freerds_read_logoff_user(wStream* s, RDS_MSG_LOGOFF_USER* msg);
int freerds_write_logoff_user(wStream* s, RDS_MSG_LOGOFF_USER* msg);

#ifdef __cplusplus
}
#endif

/* Client Message Types */

#define RDS_CLIENT_CAPABILITIES			102
#define RDS_CLIENT_REFRESH_RECT			103
#define RDS_CLIENT_SYNCHRONIZE_KEYBOARD_EVENT	104
#define RDS_CLIENT_SCANCODE_KEYBOARD_EVENT	105
#define RDS_CLIENT_VIRTUAL_KEYBOARD_EVENT	106
#define RDS_CLIENT_UNICODE_KEYBOARD_EVENT	107
#define RDS_CLIENT_MOUSE_EVENT			108
#define RDS_CLIENT_EXTENDED_MOUSE_EVENT		109
#define RDS_CLIENT_VBLANK_EVENT			110
#define RDS_CLIENT_LOGON_USER			111
#define RDS_CLIENT_LOGOFF_USER			112

struct _RDS_MSG_SYNCHRONIZE_KEYBOARD_EVENT
{
	DEFINE_MSG_COMMON();

	UINT32 flags;
};
typedef struct _RDS_MSG_SYNCHRONIZE_KEYBOARD_EVENT RDS_MSG_SYNCHRONIZE_KEYBOARD_EVENT;

struct _RDS_MSG_SCANCODE_KEYBOARD_EVENT
{
	DEFINE_MSG_COMMON();

	UINT32 flags;
	UINT32 code;
	UINT32 keyboardType;
};
typedef struct _RDS_MSG_SCANCODE_KEYBOARD_EVENT RDS_MSG_SCANCODE_KEYBOARD_EVENT;

struct _RDS_MSG_VIRTUAL_KEYBOARD_EVENT
{
	DEFINE_MSG_COMMON();

	UINT32 flags;
	UINT32 code;
};
typedef struct _RDS_MSG_VIRTUAL_KEYBOARD_EVENT RDS_MSG_VIRTUAL_KEYBOARD_EVENT;

struct _RDS_MSG_UNICODE_KEYBOARD_EVENT
{
	DEFINE_MSG_COMMON();

	UINT32 flags;
	UINT32 code;
};
typedef struct _RDS_MSG_UNICODE_KEYBOARD_EVENT RDS_MSG_UNICODE_KEYBOARD_EVENT;

struct _RDS_MSG_MOUSE_EVENT
{
	DEFINE_MSG_COMMON();

	DWORD flags;
	DWORD x;
	DWORD y;
};
typedef struct _RDS_MSG_MOUSE_EVENT RDS_MSG_MOUSE_EVENT;

struct _RDS_MSG_EXTENDED_MOUSE_EVENT
{
	DEFINE_MSG_COMMON();

	DWORD flags;
	DWORD x;
	DWORD y;
};
typedef struct _RDS_MSG_EXTENDED_MOUSE_EVENT RDS_MSG_EXTENDED_MOUSE_EVENT;

struct _RDS_MSG_CAPABILITIES
{
	DEFINE_MSG_COMMON();

	UINT32 Version;
	UINT32 DesktopWidth;
	UINT32 DesktopHeight;
	UINT32 ColorDepth;
	UINT32 KeyboardLayout;
	UINT32 KeyboardSubType;
};
typedef struct _RDS_MSG_CAPABILITIES RDS_MSG_CAPABILITIES;

struct _RDS_MSG_REFRESH_RECT
{
	DEFINE_MSG_COMMON();

	UINT32 numberOfAreas;
	RECTANGLE_16* areasToRefresh;
};
typedef struct _RDS_MSG_REFRESH_RECT RDS_MSG_REFRESH_RECT;

struct _RDS_MSG_VBLANK_EVENT
{
	DEFINE_MSG_COMMON();
};
typedef struct _RDS_MSG_VBLANK_EVENT RDS_MSG_VBLANK_EVENT;


#ifdef __cplusplus
extern "C" {
#endif

int freerds_read_synchronize_keyboard_event(wStream* s, RDS_MSG_SYNCHRONIZE_KEYBOARD_EVENT* msg);
int freerds_write_synchronize_keyboard_event(wStream* s, RDS_MSG_SYNCHRONIZE_KEYBOARD_EVENT* msg);

int freerds_read_virtual_keyboard_event(wStream* s, RDS_MSG_VIRTUAL_KEYBOARD_EVENT* msg);
int freerds_write_virtual_keyboard_event(wStream* s, RDS_MSG_VIRTUAL_KEYBOARD_EVENT* msg);

int freerds_read_scancode_keyboard_event(wStream* s, RDS_MSG_SCANCODE_KEYBOARD_EVENT* msg);
int freerds_write_scancode_keyboard_event(wStream* s, RDS_MSG_SCANCODE_KEYBOARD_EVENT* msg);

int freerds_read_unicode_keyboard_event(wStream* s, RDS_MSG_UNICODE_KEYBOARD_EVENT* msg);
int freerds_write_unicode_keyboard_event(wStream* s, RDS_MSG_UNICODE_KEYBOARD_EVENT* msg);

int freerds_read_mouse_event(wStream* s, RDS_MSG_MOUSE_EVENT* msg);
int freerds_write_mouse_event(wStream* s, RDS_MSG_MOUSE_EVENT* msg);

int freerds_read_extended_mouse_event(wStream* s, RDS_MSG_EXTENDED_MOUSE_EVENT* msg);
int freerds_write_extended_mouse_event(wStream* s, RDS_MSG_EXTENDED_MOUSE_EVENT* msg);

int freerds_read_capabilities(wStream* s, RDS_MSG_CAPABILITIES* msg);
int freerds_write_capabilities(wStream* s, RDS_MSG_CAPABILITIES* msg);

int freerds_read_refresh_rect(wStream* s, RDS_MSG_REFRESH_RECT* msg);
int freerds_write_refresh_rect(wStream* s, RDS_MSG_REFRESH_RECT* msg);

int freerds_read_vblank_event(wStream* s, RDS_MSG_VBLANK_EVENT* msg);
int freerds_write_vblank_event(wStream* s, RDS_MSG_VBLANK_EVENT* msg);

#ifdef __cplusplus
}
#endif


/* Server Message Types */

#define RDS_SERVER_BEGIN_UPDATE			1
#define RDS_SERVER_END_UPDATE			2
#define RDS_SERVER_SET_CLIPPING_REGION		3
#define RDS_SERVER_OPAQUE_RECT			4
#define RDS_SERVER_SCREEN_BLT			5
#define RDS_SERVER_PAINT_RECT			6
#define RDS_SERVER_PATBLT			7
#define RDS_SERVER_DSTBLT			8
#define RDS_SERVER_LINE_TO			9
#define RDS_SERVER_CREATE_OFFSCREEN_SURFACE	10
#define RDS_SERVER_SWITCH_OFFSCREEN_SURFACE	11
#define RDS_SERVER_DELETE_OFFSCREEN_SURFACE	12
#define RDS_SERVER_PAINT_OFFSCREEN_SURFACE	13
#define RDS_SERVER_SET_PALETTE			14
#define RDS_SERVER_CACHE_GLYPH			15
#define RDS_SERVER_GLYPH_INDEX			16
#define RDS_SERVER_SET_POINTER			17
#define RDS_SERVER_SHARED_FRAMEBUFFER		18
#define RDS_SERVER_BEEP				19
#define RDS_SERVER_RESET			20
#define RDS_SERVER_WINDOW_NEW_UPDATE		21
#define RDS_SERVER_WINDOW_DELETE		22
#define RDS_SERVER_SET_SYSTEM_POINTER		23
#define RDS_SERVER_LOGON_USER			24
#define RDS_SERVER_LOGOFF_USER			25

struct _RDS_MSG_BEGIN_UPDATE
{
	DEFINE_MSG_COMMON();
};
typedef struct _RDS_MSG_BEGIN_UPDATE RDS_MSG_BEGIN_UPDATE;

struct _RDS_MSG_END_UPDATE
{
	DEFINE_MSG_COMMON();
};
typedef struct _RDS_MSG_END_UPDATE RDS_MSG_END_UPDATE;

struct _RDS_MSG_OPAQUE_RECT
{
	DEFINE_MSG_COMMON();

	INT32 nLeftRect;
	INT32 nTopRect;
	INT32 nWidth;
	INT32 nHeight;
	UINT32 color;
};
typedef struct _RDS_MSG_OPAQUE_RECT RDS_MSG_OPAQUE_RECT;

struct _RDS_MSG_SCREEN_BLT
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
typedef struct _RDS_MSG_SCREEN_BLT RDS_MSG_SCREEN_BLT;

struct _RDS_MSG_PAINT_RECT
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
	RDS_FRAMEBUFFER* framebuffer;
};
typedef struct _RDS_MSG_PAINT_RECT RDS_MSG_PAINT_RECT;

struct _RDS_MSG_DSTBLT
{
	DEFINE_MSG_COMMON();

	INT32 nLeftRect;
	INT32 nTopRect;
	INT32 nWidth;
	INT32 nHeight;
	UINT32 bRop;
};
typedef struct _RDS_MSG_DSTBLT RDS_MSG_DSTBLT;

struct _RDS_MSG_PATBLT
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
typedef struct _RDS_MSG_PATBLT RDS_MSG_PATBLT;

struct _RDS_MSG_SET_CLIPPING_REGION
{
	DEFINE_MSG_COMMON();

	BOOL bNullRegion;
	INT32 nLeftRect;
	INT32 nTopRect;
	INT32 nWidth;
	INT32 nHeight;
};
typedef struct _RDS_MSG_SET_CLIPPING_REGION RDS_MSG_SET_CLIPPING_REGION;

struct _RDS_MSG_LINE_TO
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
typedef struct _RDS_MSG_LINE_TO RDS_MSG_LINE_TO;

struct _RDS_MSG_SET_POINTER
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
typedef struct _RDS_MSG_SET_POINTER RDS_MSG_SET_POINTER;

struct _RDS_MSG_SET_SYSTEM_POINTER
{
	DEFINE_MSG_COMMON();

	UINT32 ptrType;
};
typedef struct _RDS_MSG_SET_SYSTEM_POINTER RDS_MSG_SET_SYSTEM_POINTER;

struct _RDS_MSG_SET_PALETTE
{
	DEFINE_MSG_COMMON();

	UINT32* palette;
};
typedef struct _RDS_MSG_SET_PALETTE RDS_MSG_SET_PALETTE;

struct _RDS_MSG_CREATE_OFFSCREEN_SURFACE
{
	DEFINE_MSG_COMMON();

	UINT32 cacheIndex;
	UINT32 nWidth;
	UINT32 nHeight;
};
typedef struct _RDS_MSG_CREATE_OFFSCREEN_SURFACE RDS_MSG_CREATE_OFFSCREEN_SURFACE;

struct _RDS_MSG_SWITCH_OFFSCREEN_SURFACE
{
	DEFINE_MSG_COMMON();

	UINT32 cacheIndex;
};
typedef struct _RDS_MSG_SWITCH_OFFSCREEN_SURFACE RDS_MSG_SWITCH_OFFSCREEN_SURFACE;

struct _RDS_MSG_DELETE_OFFSCREEN_SURFACE
{
	DEFINE_MSG_COMMON();

	UINT32 cacheIndex;
};
typedef struct _RDS_MSG_DELETE_OFFSCREEN_SURFACE RDS_MSG_DELETE_OFFSCREEN_SURFACE;

struct _RDS_MSG_PAINT_OFFSCREEN_SURFACE
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
typedef struct _RDS_MSG_PAINT_OFFSCREEN_SURFACE RDS_MSG_PAINT_OFFSCREEN_SURFACE;

struct _RDS_GLYPH_DATA
{
	UINT32 cacheIndex;
	INT32 x;
	INT32 y;
	UINT32 cx;
	UINT32 cy;
	UINT32 cb;
	BYTE* aj;
};
typedef struct _RDS_GLYPH_DATA RDS_GLYPH_DATA;

struct _RDS_MSG_CACHE_GLYPH
{
	DEFINE_MSG_COMMON();

	UINT32 cacheId;
	UINT32 flags;
	UINT32 cGlyphs;
	RDS_GLYPH_DATA* glyphData;
	BYTE* unicodeCharacters;
};
typedef struct _RDS_MSG_CACHE_GLYPH RDS_MSG_CACHE_GLYPH;

struct _RDS_MSG_GLYPH_INDEX
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
typedef struct _RDS_MSG_GLYPH_INDEX RDS_MSG_GLYPH_INDEX;

struct _RDS_MSG_RESET
{
	DEFINE_MSG_COMMON();

	UINT32 DesktopWidth;
	UINT32 DesktopHeight;
	UINT32 ColorDepth;
};
typedef struct _RDS_MSG_RESET RDS_MSG_RESET;

struct _RDS_MSG_BEEP
{
	DEFINE_MSG_COMMON();

};
typedef struct _RDS_MSG_BEEP RDS_MSG_BEEP;

struct _RDS_MSG_WINDOW_NEW_UPDATE
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
typedef struct _RDS_MSG_WINDOW_NEW_UPDATE RDS_MSG_WINDOW_NEW_UPDATE;

struct _RDS_MSG_WINDOW_DELETE
{
	DEFINE_MSG_COMMON();

	UINT32 windowId;
};
typedef struct _RDS_MSG_WINDOW_DELETE RDS_MSG_WINDOW_DELETE;

struct _RDS_MSG_SHARED_FRAMEBUFFER
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
typedef struct _RDS_MSG_SHARED_FRAMEBUFFER RDS_MSG_SHARED_FRAMEBUFFER;

union _RDS_MSG_SERVER
{
	RDS_MSG_BEGIN_UPDATE BeginUpdate;
	RDS_MSG_END_UPDATE EndUpdate;
	RDS_MSG_SET_CLIPPING_REGION SetClippingRegion;
	RDS_MSG_OPAQUE_RECT OpaqueRect;
	RDS_MSG_SCREEN_BLT ScreenBlt;
	RDS_MSG_PAINT_RECT PaintRect;
	RDS_MSG_PATBLT PatBlt;
	RDS_MSG_DSTBLT DstBlt;
	RDS_MSG_LINE_TO LineTo;
	RDS_MSG_CREATE_OFFSCREEN_SURFACE CreateOffscreenSurface;
	RDS_MSG_SWITCH_OFFSCREEN_SURFACE SwitchOffscreenSurface;
	RDS_MSG_DELETE_OFFSCREEN_SURFACE DeleteOffscreenSurface;
	RDS_MSG_PAINT_OFFSCREEN_SURFACE PaintOffscreenSurface;
	RDS_MSG_SET_PALETTE SetPalette;
	RDS_MSG_CACHE_GLYPH CacheGlyph;
	RDS_MSG_GLYPH_INDEX GlyphIndex;
	RDS_MSG_SET_POINTER SetPointer;
	RDS_MSG_SET_SYSTEM_POINTER SetSystemPointer;
	RDS_MSG_SHARED_FRAMEBUFFER SharedFramebuffer;
	RDS_MSG_BEEP Beep;
	RDS_MSG_RESET Reset;
	RDS_MSG_WINDOW_NEW_UPDATE WindowNewUpdate;
	RDS_MSG_WINDOW_DELETE WindowDelete;
};
typedef union _RDS_MSG_SERVER RDS_MSG_SERVER;

/**
 * Module Interface
 */

typedef int (*pRdsGetEventHandles)(rdsModuleConnector *connector, HANDLE* events, DWORD* nCount);
typedef int (*pRdsCheckEventHandles)(rdsModuleConnector* connector);

typedef int (*pRdsClientSynchronizeKeyboardEvent)(rdsModuleConnector* connector, DWORD flags);
typedef int (*pRdsClientScancodeKeyboardEvent)(rdsModuleConnector* connector, DWORD flags, DWORD code, DWORD keyboardType);
typedef int (*pRdsClientVirtualKeyboardEvent)(rdsModuleConnector* connector, DWORD flags, DWORD code);
typedef int (*pRdsClientUnicodeKeyboardEvent)(rdsModuleConnector* connector, DWORD flags, DWORD code);
typedef int (*pRdsClientMouseEvent)(rdsModuleConnector* connector, DWORD flags, DWORD x, DWORD y);
typedef int (*pRdsClientExtendedMouseEvent)(rdsModuleConnector* connector, DWORD flags, DWORD x, DWORD y);
typedef int (*pRdsClientVBlankEvent)(rdsModuleConnector* connector);
typedef int (*pRdsClientLogonUser)(rdsModuleConnector* connector, RDS_MSG_LOGON_USER* msg);
typedef int (*pRdsClientLogoffUser)(rdsModuleConnector* connector, RDS_MSG_LOGOFF_USER* msg);

struct rds_client_interface
{
	pRdsClientSynchronizeKeyboardEvent SynchronizeKeyboardEvent;
	pRdsClientScancodeKeyboardEvent ScancodeKeyboardEvent;
	pRdsClientVirtualKeyboardEvent VirtualKeyboardEvent;
	pRdsClientUnicodeKeyboardEvent UnicodeKeyboardEvent;
	pRdsClientMouseEvent MouseEvent;
	pRdsClientExtendedMouseEvent ExtendedMouseEvent;
	pRdsClientVBlankEvent VBlankEvent;
	pRdsClientLogonUser LogonUser;
	pRdsClientLogoffUser LogoffUser;
};
typedef struct rds_client_interface rdsClientInterface;

typedef int (*pRdsServerIsTerminated)(rdsModuleConnector* connector);

typedef int (*pRdsServerBeginUpdate)(rdsModuleConnector* connector, RDS_MSG_BEGIN_UPDATE* msg);
typedef int (*pRdsServerEndUpdate)(rdsModuleConnector* connector, RDS_MSG_END_UPDATE* msg);
typedef int (*pRdsServerBeep)(rdsModuleConnector* connector, RDS_MSG_BEEP* msg);
typedef int (*pRdsServerOpaqueRect)(rdsModuleConnector* connector, RDS_MSG_OPAQUE_RECT* msg);
typedef int (*pRdsServerScreenBlt)(rdsModuleConnector* connector, RDS_MSG_SCREEN_BLT* msg);
typedef int (*pRdsServerPaintRect)(rdsModuleConnector* connector, RDS_MSG_PAINT_RECT* msg);
typedef int (*pRdsServerPatBlt)(rdsModuleConnector* connector, RDS_MSG_PATBLT* msg);
typedef int (*pRdsServerDstBlt)(rdsModuleConnector* connector, RDS_MSG_DSTBLT* msg);
typedef int (*pRdsServerSetPointer)(rdsModuleConnector* connector, RDS_MSG_SET_POINTER* msg);
typedef int (*pRdsServerSetSystemPointer)(rdsModuleConnector* connector, RDS_MSG_SET_SYSTEM_POINTER* msg);
typedef int (*pRdsServerSetPalette)(rdsModuleConnector* connector, RDS_MSG_SET_PALETTE* msg);
typedef int (*pRdsServerSetClippingRegion)(rdsModuleConnector* connector, RDS_MSG_SET_CLIPPING_REGION* msg);
typedef int (*pRdsServerLineTo)(rdsModuleConnector* connector, RDS_MSG_LINE_TO* msg);
typedef int (*pRdsServerCacheGlyph)(rdsModuleConnector* connector, RDS_MSG_CACHE_GLYPH* msg);
typedef int (*pRdsServerGlyphIndex)(rdsModuleConnector* connector, RDS_MSG_GLYPH_INDEX* msg);
typedef int (*pRdsServerSharedFramebuffer)(rdsModuleConnector* connector, RDS_MSG_SHARED_FRAMEBUFFER* msg);
typedef int (*pRdsServerReset)(rdsModuleConnector* connector, RDS_MSG_RESET* msg);
typedef int (*pRdsServerCreateOffscreenSurface)(rdsModuleConnector* connector, RDS_MSG_CREATE_OFFSCREEN_SURFACE* msg);
typedef int (*pRdsServerSwitchOffscreenSurface)(rdsModuleConnector* connector, RDS_MSG_SWITCH_OFFSCREEN_SURFACE* msg);
typedef int (*pRdsServerDeleteOffscreenSurface)(rdsModuleConnector* connector, RDS_MSG_DELETE_OFFSCREEN_SURFACE* msg);
typedef int (*pRdsServerPaintOffscreenSurface)(rdsModuleConnector* connector, RDS_MSG_PAINT_OFFSCREEN_SURFACE* msg);

typedef int (*pRdsServerWindowNewUpdate)(rdsModuleConnector* connector, RDS_MSG_WINDOW_NEW_UPDATE* msg);
typedef int (*pRdsServerWindowDelete)(rdsModuleConnector* connector, RDS_MSG_WINDOW_DELETE* msg);

typedef int (*pRdsServerLogonUser)(rdsModuleConnector* connector, RDS_MSG_LOGON_USER* msg);
typedef int (*pRdsServerLogoffUser)(rdsModuleConnector* connector, RDS_MSG_LOGOFF_USER* msg);

struct rds_server_interface
{
	pRdsServerBeginUpdate BeginUpdate;
	pRdsServerEndUpdate EndUpdate;
	pRdsServerBeep Beep;
	pRdsServerIsTerminated IsTerminated;
	pRdsServerOpaqueRect OpaqueRect;
	pRdsServerScreenBlt ScreenBlt;
	pRdsServerPaintRect PaintRect;
	pRdsServerPatBlt PatBlt;
	pRdsServerDstBlt DstBlt;
	pRdsServerSetPointer SetPointer;
	pRdsServerSetSystemPointer SetSystemPointer;
	pRdsServerSetPalette SetPalette;
	pRdsServerSetClippingRegion SetClippingRegion;
	pRdsServerLineTo LineTo;
	pRdsServerCacheGlyph CacheGlyph;
	pRdsServerGlyphIndex GlyphIndex;
	pRdsServerSharedFramebuffer SharedFramebuffer;
	pRdsServerReset Reset;
	pRdsServerCreateOffscreenSurface CreateOffscreenSurface;
	pRdsServerSwitchOffscreenSurface SwitchOffscreenSurface;
	pRdsServerDeleteOffscreenSurface DeleteOffscreenSurface;
	pRdsServerPaintOffscreenSurface PaintOffscreenSurface;
	pRdsServerWindowNewUpdate WindowNewUpdate;
	pRdsServerWindowDelete WindowDelete;
	pRdsServerLogonUser LogonUser;
	pRdsServerLogoffUser LogoffUser;
};
typedef struct rds_server_interface rdsServerInterface;

struct rds_module_connector
{
	DWORD Size;
	char* Endpoint;
	DWORD SessionId;
	BOOL ServerMode;
	rdsClientInterface* client;
	rdsServerInterface* server;
	HANDLE hClientPipe;
	HANDLE hServerPipe;
	wStream* OutboundStream;
	wStream* InboundStream;
	UINT32 InboundTotalLength;
	UINT32 InboundTotalCount;
	UINT32 OutboundTotalLength;
	UINT32 OutboundTotalCount;
	pRdsGetEventHandles GetEventHandles;
	pRdsCheckEventHandles CheckEventHandles;

	freerdp* instance;
	rdpSettings* settings;
	rdsConnection* connection;
	HANDLE SocketEvent;

	RDS_FRAMEBUFFER framebuffer;

	int fps;
	int MaxFps;
	HANDLE StopEvent;
	HANDLE ServerTimer;
	HANDLE ServerThread;
	wLinkedList* ServerList;
	wMessageQueue* ServerQueue;
	rdsServerInterface* ServerProxy;
};

#ifdef __cplusplus
extern "C" {
#endif

FREERDP_API int freerds_client_message_size(UINT32 type);
FREERDP_API char* freerds_client_message_name(UINT32 type);

FREERDP_API int freerds_client_message_read(wStream* s, RDS_MSG_COMMON* msg);
FREERDP_API int freerds_client_message_write(wStream* s, RDS_MSG_COMMON* msg);

FREERDP_API void* freerds_client_message_copy(RDS_MSG_COMMON* msg);
FREERDP_API void freerds_client_message_free(RDS_MSG_COMMON* msg);

FREERDP_API int freerds_server_message_size(UINT32 type);
FREERDP_API char* freerds_server_message_name(UINT32 type);

FREERDP_API int freerds_server_message_read(wStream* s, RDS_MSG_COMMON* msg);
FREERDP_API int freerds_server_message_write(wStream* s, RDS_MSG_COMMON* msg);

FREERDP_API void* freerds_server_message_copy(RDS_MSG_COMMON* msg);
FREERDP_API void freerds_server_message_free(RDS_MSG_COMMON* msg);

/**
 * New Clean Module Interface API
 */

FREERDP_API rdsClientInterface* freerds_client_outbound_interface_new(void);
FREERDP_API rdsServerInterface* freerds_server_outbound_interface_new(void);

FREERDP_API rdsServerInterface* freerds_client_inbound_interface_new(void);
FREERDP_API rdsClientInterface* freerds_server_inbound_interface_new(void);

FREERDP_API int freerds_named_pipe_read(HANDLE hNamedPipe, BYTE* data, DWORD length);
FREERDP_API int freerds_named_pipe_write(HANDLE hNamedPipe, BYTE* data, DWORD length);

FREERDP_API int freerds_server_outbound_write_message(rdsModuleConnector* connector, RDS_MSG_COMMON* msg);

FREERDP_API void freerds_named_pipe_get_endpoint_name(DWORD id, const char *endpoint, char *dest, int len);
FREERDP_API int freerds_named_pipe_clean(const char* pipeName);
FREERDP_API HANDLE freerds_named_pipe_connect(const char* pipeName, DWORD nTimeOut);
FREERDP_API HANDLE freerds_named_pipe_create(const char* pipeName);

FREERDP_API int freerds_named_pipe_clean_endpoint(DWORD id, const char* endpoint);
FREERDP_API HANDLE freerds_named_pipe_connect_endpoint(DWORD id, const char* endpoint, DWORD nTimeOut);
FREERDP_API HANDLE freerds_named_pipe_create_endpoint(DWORD id, const char* endpoint);
FREERDP_API HANDLE freerds_named_pipe_accept(HANDLE hServerPipe);

FREERDP_API int freerds_transport_receive(rdsModuleConnector* connector);

#ifdef __cplusplus
}
#endif

#endif /* FREERDS_H */
