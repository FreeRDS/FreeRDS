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
 * main include file
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef XRDP_H
#define XRDP_H

#include <winpr/crt.h>
#include <winpr/synch.h>
#include <winpr/stream.h>

#include <freerdp/freerdp.h>
#include <freerdp/listener.h>

#include <xrdp-ng/xrdp.h>

#include <pixman.h>

#include "trans.h"
#include "list.h"
#include "defines.h"
#include "os_calls.h"
#include "thread_calls.h"
#include "file.h"

typedef struct xrdp_listener xrdpListener;
typedef struct xrdp_mod xrdpModule;
typedef struct xrdp_mm xrdpMm;

#include "xrdp_core.h"

struct xrdp_mm
{
	xrdpSession* session;
	xrdpModule* mod;
	int display;
};

int g_is_term(void);
void g_set_term(int in_val);
HANDLE g_get_term_event(void);

xrdpSession* xrdp_process_create(freerdp_peer* client);
void xrdp_process_delete(xrdpSession* self);
HANDLE xrdp_process_get_term_event(xrdpSession* self);
void* xrdp_process_main_thread(void* arg);

xrdpListener* xrdp_listen_create(void);
void xrdp_listen_delete(xrdpListener* self);
int xrdp_listen_main_loop(xrdpListener* self);

xrdpModule* xrdp_module_new(xrdpSession* session);
void xrdp_module_free(xrdpModule* mod);

long xrdp_authenticate(char* username, char* password, int* errorcode);

int xrdp_client_module_init(xrdpModule* mod);
int xrdp_client_module_uninit(xrdpModule* mod);

int xrdp_server_module_init(xrdpModule* mod);
int xrdp_message_server_module_init(xrdpModule* mod);

int xrdp_message_server_queue_pack(xrdpModule* mod);
int xrdp_message_server_queue_process_pending_messages(xrdpModule* mod);
int xrdp_message_server_module_init(xrdpModule* mod);

/**
 * Module Interface
 */

typedef int (*pXrdpClientStart)(xrdpModule* mod, int width, int height, int bpp);
typedef int (*pXrdpClientConnect)(xrdpModule* mod);
typedef int (*pXrdpClientSynchronizeKeyboardEvent)(xrdpModule* mod, DWORD flags);
typedef int (*pXrdpClientScancodeKeyboardEvent)(xrdpModule* mod, DWORD flags, DWORD code, DWORD keyboardType);
typedef int (*pXrdpClientVirtualKeyboardEvent)(xrdpModule* mod, DWORD flags, DWORD code);
typedef int (*pXrdpClientUnicodeKeyboardEvent)(xrdpModule* mod, DWORD flags, DWORD code);
typedef int (*pXrdpClientMouseEvent)(xrdpModule* mod, DWORD flags, DWORD x, DWORD y);
typedef int (*pXrdpClientExtendedMouseEvent)(xrdpModule* mod, DWORD flags, DWORD x, DWORD y);
typedef int (*pXrdpClientEnd)(xrdpModule* mod);
typedef int (*pXrdpClientSessionChange)(xrdpModule* mod, int width, int height);
typedef int (*pXrdpClientGetEventHandles)(xrdpModule* mod, HANDLE* events, DWORD* nCount);
typedef int (*pXrdpClientCheckEventHandles)(xrdpModule* mod);

struct xrdp_client_module
{
	pXrdpClientStart Start;
	pXrdpClientConnect Connect;
	pXrdpClientSynchronizeKeyboardEvent SynchronizeKeyboardEvent;
	pXrdpClientScancodeKeyboardEvent ScancodeKeyboardEvent;
	pXrdpClientVirtualKeyboardEvent VirtualKeyboardEvent;
	pXrdpClientUnicodeKeyboardEvent UnicodeKeyboardEvent;
	pXrdpClientMouseEvent MouseEvent;
	pXrdpClientExtendedMouseEvent ExtendedMouseEvent;
	pXrdpClientEnd End;
	pXrdpClientSessionChange SessionChange;
	pXrdpClientGetEventHandles GetEventHandles;
	pXrdpClientCheckEventHandles CheckEventHandles;
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
	int version;

	xrdpClientModule* client;
	xrdpServerModule* server;

	long handle;

	int width;
	int height;
	int bpp;

	DWORD SessionId;
	xrdpSession* session;

	freerdp* instance;
	rdpSettings* settings;

	HANDLE hClientPipe;

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
};

#endif /* XRDP_H */
