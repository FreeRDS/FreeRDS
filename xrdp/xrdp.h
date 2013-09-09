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

#include "core.h"
#include "defines.h"
#include "os_calls.h"
#include "thread_calls.h"
#include "file.h"

typedef struct xrdp_listener xrdpListener;
typedef struct xrdp_mod xrdpModule;
typedef struct xrdp_mm xrdpMm;
typedef struct xrdp_wm xrdpWm;

struct xrdp_mm
{
	xrdpWm* wm;
	xrdpSession* session;
	int connected_state; /* true if connected to sesman else false */
	struct trans* sesman_trans; /* connection to sesman */
	int sesman_trans_up; /* true once connected to sesman */
	int delete_sesman_trans; /* boolean set when done with sesman connection */
	/* mod vars */
	long mod_handle;
	int (*ModuleInit)(xrdpModule*);
	int (*ModuleExit)(xrdpModule*);
	xrdpModule* mod; /* module interface */
	int display; /* 10 for :10.0, 11 for :11.0, etc */
	int code; /* 0 Xvnc session 10 X11rdp session */
	int sesman_controlled; /* true if this is a sesman session */
};

/* the window manager */

struct xrdp_wm
{
	xrdpSession* session;
	xrdpMm* mm;
};

/* xrdp.c */
long g_xrdp_sync(long (*sync_func)(long param1, long param2), long sync_param1, long sync_param2);
int g_is_term(void);
void g_set_term(int in_val);
HANDLE g_get_term_event(void);
HANDLE g_get_sync_event(void);
void g_process_waiting_function(void);

/* xrdp_process.c */
xrdpSession* xrdp_process_create(freerdp_peer* client);
void xrdp_process_delete(xrdpSession* self);
HANDLE xrdp_process_get_term_event(xrdpSession* self);
void* xrdp_process_main_thread(void* arg);

/* xrdp_listen.c */
xrdpListener* xrdp_listen_create(void);
void xrdp_listen_delete(xrdpListener* self);
int xrdp_listen_main_loop(xrdpListener* self);

/* xrdp_mm.c */
xrdpMm* xrdp_mm_create(xrdpSession* session);
void xrdp_mm_delete(xrdpMm* self);
int xrdp_mm_connect(xrdpMm* self);
int xrdp_mm_setup_mod1(xrdpMm* self);
int xrdp_mm_setup_mod2(xrdpMm* self);
void xrdp_mm_cleanup_sesman_connection(xrdpMm* self);
int xrdp_mm_process_channel_data(xrdpMm* self, LONG_PTR param1, LONG_PTR param2, LONG_PTR param3, LONG_PTR param4);
int xrdp_mm_get_event_handles(xrdpMm* self, HANDLE* events, DWORD* nCount);
int xrdp_mm_check_wait_objs(xrdpMm* self);

/* xrdp_auth.c */
int xrdp_mm_send_login(xrdpMm* self);
int xrdp_mm_process_login_response(xrdpMm* self, wStream* s);

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
typedef int (*pXrdpClientEvent)(xrdpModule* mod, int msg, long param1, long param2, long param3, long param4);
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
	pXrdpClientEvent Event;
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

	int sck;
	int sck_closed;
	char ip[256];
	char port[256];
	int shift_state;

	int width;
	int height;
	int bpp;
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
};

#endif /* XRDP_H */
