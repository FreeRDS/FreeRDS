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
typedef struct xrdp_palette_item xrdpPaletteItem;
typedef struct xrdp_bitmap_item xrdpBitmapItem;
typedef struct xrdp_pointer_item xrdpPointerItem;
typedef struct xrdp_brush_item xrdpBrushItem;
typedef struct xrdp_mm xrdpMm;
typedef struct xrdp_key_info xrdpKeyInfo;
typedef struct xrdp_keymap xrdpKeymap;
typedef struct xrdp_wm xrdpWm;
typedef struct xrdp_region xrdpRegion;
typedef struct xrdp_bitmap xrdpBitmap;
typedef struct xrdp_mod_data xrdpModuleData;
typedef struct xrdp_startup_params xrdpStartupParams;

#define DEFAULT_STRING_LEN 255
#define LOG_WINDOW_CHAR_PER_LINE 60

#define MAX_NR_CHANNELS 16
#define MAX_CHANNEL_NAME 16

struct bitmap_item
{
	int width;
	int height;
	char* data;
};

struct brush_item
{
	int bpp;
	int width;
	int height;
	char* data;
	char b8x8[8];
};

struct pointer_item
{
	int hotx;
	int hoty;
	char data[32 * 32 * 3];
	char mask[32 * 32 / 8];
};

struct xrdp_palette_item
{
	int stamp;
	int palette[256];
};

struct xrdp_bitmap_item
{
	int stamp;
	xrdpBitmap* bitmap;
};

struct xrdp_pointer_item
{
	int stamp;
	int x; /* hotspot */
	int y;
	char data[32 * 32 * 4];
	char mask[32 * 32 / 8];
	int bpp;
};

struct xrdp_brush_item
{
	int stamp;
	/* expand this to a structure to handle more complicated brushes
	 for now its 8x8 1bpp brushes only */
	char pattern[8];
};

struct xrdp_mm
{
	xrdpWm* wm; /* owner */
	int connected_state; /* true if connected to sesman else false */
	struct trans* sesman_trans; /* connection to sesman */
	int sesman_trans_up; /* true once connected to sesman */
	int delete_sesman_trans; /* boolean set when done with sesman connection */
	xrdpList* login_names;
	xrdpList* login_values;
	/* mod vars */
	long mod_handle;
	int (*ModuleInit)(xrdpModule*);
	int (*ModuleExit)(xrdpModule*);
	xrdpModule* mod; /* module interface */
	int display; /* 10 for :10.0, 11 for :11.0, etc */
	int code; /* 0 Xvnc session 10 X11rdp session */
	int sesman_controlled; /* true if this is a sesman session */
};

struct xrdp_key_info
{
	int sym;
	int chr;
};

struct xrdp_keymap
{
	xrdpKeyInfo keys_noshift[256];
	xrdpKeyInfo keys_shift[256];
	xrdpKeyInfo keys_altgr[256];
	xrdpKeyInfo keys_capslock[256];
	xrdpKeyInfo keys_shiftcapslock[256];
};

/* the window manager */

struct xrdp_wm
{
	xrdpSession* pro_layer; /* owner */
	xrdpBitmap* screen;
	xrdpSession* session;
	/* keyboard info */
	int keys[256]; /* key states 0 up 1 down*/
	int caps_lock;
	int scroll_lock;
	int num_lock;
	/* session log */
	xrdpList* log;
	xrdpMm* mm;
	xrdpKeymap keymap;
	char pamerrortxt[256];
};

/* region */
struct xrdp_region
{
	xrdpWm* wm; /* owner */
	xrdpList* rects;
};

/* window or bitmap */
struct xrdp_bitmap
{
	int type;
	int width;
	int height;
	xrdpWm* wm;
	/* for bitmap */
	int bpp;
	int line_size; /* in bytes */
	int do_not_free_data;
	char* data;
	/* for all but bitmap */
	int left;
	int top;
	int pointer;
	int bg_color;
	int tab_stop;
	int id;
	int item_index;
	int item_height;
	int crc;
};

/* module */
struct xrdp_mod_data
{
	xrdpList* names;
	xrdpList* values;
};

struct xrdp_startup_params
{
	char port[128];
	int kill;
	int no_daemon;
	int help;
	int version;
	int fork;
};

/* drawable types */
#define WND_TYPE_BITMAP		0
#define WND_TYPE_WND		1
#define WND_TYPE_SCREEN		2
#define WND_TYPE_OFFSCREEN	10

/* button states */
#define BUTTON_STATE_UP		0
#define BUTTON_STATE_DOWN	1

/* messages */
#define WM_XRDP_PAINT		3
#define WM_XRDP_KEYDOWN		15
#define WM_XRDP_KEYUP		16
#define WM_XRDP_MOUSEMOVE	100
#define WM_XRDP_LBUTTONUP	101
#define WM_XRDP_LBUTTONDOWN	102
#define WM_XRDP_RBUTTONUP	103
#define WM_XRDP_RBUTTONDOWN	104
#define WM_XRDP_BUTTON3UP	105
#define WM_XRDP_BUTTON3DOWN	106
#define WM_XRDP_BUTTON4UP	107
#define WM_XRDP_BUTTON4DOWN	108
#define WM_XRDP_BUTTON5UP	109
#define WM_XRDP_BUTTON5DOWN	110
#define WM_XRDP_INVALIDATE	200

/* xrdp.c */
long g_xrdp_sync(long (*sync_func)(long param1, long param2), long sync_param1, long sync_param2);
int g_is_term(void);
void g_set_term(int in_val);
HANDLE g_get_term_event(void);
HANDLE g_get_sync_event(void);
void g_process_waiting_function(void);

/* xrdp_wm.c */
xrdpWm* xrdp_wm_create(xrdpSession* owner);
void xrdp_wm_delete(xrdpWm* self);
int xrdp_wm_init(xrdpWm* self);
int xrdp_wm_mouse_move(xrdpWm* self, int x, int y);
int xrdp_wm_mouse_click(xrdpWm* self, int x, int y, int but, int down);
int xrdp_wm_process_input_mouse(xrdpWm *self, int device_flags, int x, int y);
int xrdp_wm_key_sync(xrdpWm* self, int device_flags, int key_flags);
int xrdp_wm_pointer(xrdpWm* self, XRDP_MSG_SET_POINTER* msg);
int xrdp_wm_get_event_handles(xrdpWm* self, HANDLE* events, DWORD* nCount);
int xrdp_wm_check_wait_objs(xrdpWm* self);
int xrdp_wm_set_login_mode(xrdpWm* self, int login_mode);

/* xrdp_process.c */
xrdpSession* xrdp_process_create(freerdp_peer* client);
void xrdp_process_delete(xrdpSession* self);
HANDLE xrdp_process_get_term_event(xrdpSession* self);
xrdpSession* xrdp_process_get_session(xrdpSession* self);
xrdpWm* xrdp_process_get_wm(xrdpSession* self);
void* xrdp_process_main_thread(void* arg);

/* xrdp_listen.c */
xrdpListener* xrdp_listen_create(void);
void xrdp_listen_delete(xrdpListener* self);
int xrdp_listen_main_loop(xrdpListener* self);

/* xrdp_region.c */
xrdpRegion* xrdp_region_create(xrdpWm* wm);
void xrdp_region_delete(xrdpRegion* self);
int xrdp_region_add_rect(xrdpRegion* self, xrdpRect* rect);
int xrdp_region_insert_rect(xrdpRegion* self, int i, int left, int top, int right, int bottom);
int xrdp_region_subtract_rect(xrdpRegion* self, xrdpRect* rect);
int xrdp_region_get_rect(xrdpRegion* self, int index, xrdpRect* rect);

/* xrdp_bitmap.c */
xrdpBitmap* xrdp_bitmap_create(int width, int height, int bpp, int type, xrdpWm* wm);
void xrdp_bitmap_delete(xrdpBitmap* self);

/* xrdp_bitmap_compress.c */
int xrdp_bitmap_compress(char* in_data, int width, int height,
		wStream* s, int bpp, int byte_limit,
		int start_line, wStream* temp, int e);

/* xrdp_mm.c */
xrdpMm* xrdp_mm_create(xrdpWm* owner);
void xrdp_mm_delete(xrdpMm* self);
int xrdp_mm_connect(xrdpMm* self);
int xrdp_mm_setup_mod1(xrdpMm* self);
int xrdp_mm_setup_mod2(xrdpMm* self);
int xrdp_mm_get_value(xrdpMm* self, char *aname, char *dest, int dest_len);
void xrdp_mm_cleanup_sesman_connection(xrdpMm* self);
int xrdp_mm_process_channel_data(xrdpMm* self, LONG_PTR param1, LONG_PTR param2, LONG_PTR param3, LONG_PTR param4);
int xrdp_mm_get_event_handles(xrdpMm* self, HANDLE* events, DWORD* nCount);
int xrdp_mm_check_wait_objs(xrdpMm* self);
int xrdp_child_fork(void);

/* xrdp_auth.c */
int xrdp_mm_send_login(xrdpMm* self);
int xrdp_mm_process_login_response(xrdpMm* self, wStream* s);
int xrdp_mm_access_control(char *username, char *password, char *srv);
const char* getPAMError(const int pamError, char *text, int text_bytes);
const char* getPAMAdditionalErrorInfo(const int pamError, xrdpMm* self);

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
typedef int (*pXrdpClientScancodeKeyboardEvent)(xrdpModule* mod, DWORD flags, DWORD code);
typedef int (*pXrdpClientUnicodeKeyboardEvent)(xrdpModule* mod, DWORD flags, DWORD code);
typedef int (*pXrdpClientEnd)(xrdpModule* mod);
typedef int (*pXrdpClientSetParam)(xrdpModule* mod, char* name, char* value);
typedef int (*pXrdpClientSessionChange)(xrdpModule* mod, int width, int height);
typedef int (*pXrdpClientGetEventHandles)(xrdpModule* mod, HANDLE* events, DWORD* nCount);
typedef int (*pXrdpClientCheckEventHandles)(xrdpModule* mod);

struct xrdp_client_module
{
	pXrdpClientStart Start;
	pXrdpClientConnect Connect;
	pXrdpClientEvent Event;
	pXrdpClientScancodeKeyboardEvent ScancodeKeyboardEvent;
	pXrdpClientUnicodeKeyboardEvent UnicodeKeyboardEvent;
	pXrdpClientEnd End;
	pXrdpClientSetParam SetParam;
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
typedef int (*pXrdpServerWindowIcon)(xrdpModule* mod, int window_id, int cache_entry, int cache_id, xrdpRailIconInfo* icon_info, int flags);
typedef int (*pXrdpServerWindowCachedIcon)(xrdpModule* mod, int window_id, int cache_entry, int cache_id, int flags);
typedef int (*pXrdpServerNotifyNewUpdate)(xrdpModule* mod, int window_id, int notify_id, xrdpRailNotifyStateOrder* notify_state, int flags);
typedef int (*pXrdpServerNotifyDelete)(xrdpModule* mod, int window_id, int notify_id);
typedef int (*pXrdpServerMonitoredDesktop)(xrdpModule* mod, xrdpRailMonitoredDesktopOrder* mdo, int flags);

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
	pXrdpServerWindowIcon WindowIcon;
	pXrdpServerWindowCachedIcon WindowCachedIcon;
	pXrdpServerNotifyNewUpdate NotifyNewUpdate;
	pXrdpServerNotifyDelete NotifyDelete;
	pXrdpServerMonitoredDesktop MonitoredDesktop;
};
typedef struct xrdp_server_module xrdpServerModule;

struct xrdp_mod
{
	int size;
	int version;

	xrdpClientModule* client;
	xrdpServerModule* server;

	/* common */
	long handle; /* pointer to self as int */
	long wm; /* xrdpWm* */
	long painter;
	int sck;
	/* mod data */
	int width;
	int height;
	int bpp;
	int rfx;
	int sck_closed;
	char username[256];
	char password[256];
	char ip[256];
	char port[256];
	long sck_obj;
	int shift_state;

	xrdpSession* session;

	rdpSettings* settings;

	UINT32 TotalLength;
	UINT32 TotalCount;
	HANDLE SocketEvent;
	wStream* SendStream;
	wStream* ReceiveStream;

	int colormap[256];
	freerdp* instance;
	struct bitmap_item bitmap_cache[4][4096];
	struct brush_item brush_cache[64];
	struct pointer_item pointer_cache[32];

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
