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

#include <xrdp-ng/xrdp.h>

#include "trans.h"
#include "list.h"

#include "core.h"
#include "defines.h"
#include "os_calls.h"
#include "thread_calls.h"
#include "file.h"

typedef struct xrdp_listener xrdpListener;
typedef struct xrdp_process xrdpProcess;
typedef struct xrdp_mod xrdpModule;
typedef struct xrdp_bmp_header xrdpBmpHeader;
typedef struct xrdp_palette_item xrdpPaletteItem;
typedef struct xrdp_bitmap_item xrdpBitmapItem;
typedef struct xrdp_os_bitmap_item xrdpOffscreenBitmapItem;
typedef struct xrdp_char_item xrdpCharItem;
typedef struct xrdp_pointer_item xrdpPointerItem;
typedef struct xrdp_brush_item xrdpBrushItem;
typedef struct xrdp_cache xrdpCache;
typedef struct xrdp_mm xrdpMm;
typedef struct xrdp_key_info xrdpKeyInfo;
typedef struct xrdp_keymap xrdpKeymap;
typedef struct xrdp_wm xrdpWm;
typedef struct xrdp_region xrdpRegion;
typedef struct xrdp_painter xrdpPainter;
typedef struct xrdp_bitmap xrdpBitmap;
typedef struct xrdp_font xrdpFont;
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

/* header for bmp file */
struct xrdp_bmp_header
{
	int size;
	int image_width;
	int image_height;
	short planes;
	short bit_count;
	int compression;
	int image_size;
	int x_pels_per_meter;
	int y_pels_per_meter;
	int clr_used;
	int clr_important;
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

struct xrdp_os_bitmap_item
{
	int id;
	xrdpBitmap* bitmap;
};

struct xrdp_char_item
{
	int stamp;
	xrdpFontChar font_item;
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

/* difference caches */
struct xrdp_cache
{
	xrdpWm* wm; /* owner */
	xrdpSession* session;
	/* palette */
	int palette_stamp;
	xrdpPaletteItem palette_items[6];
	/* bitmap */
	int bitmap_stamp;
	xrdpBitmapItem bitmap_items[3][2000];
	int BitmapCompressionDisabled;
	int cache1_entries;
	int cache1_size;
	int cache2_entries;
	int cache2_size;
	int cache3_entries;
	int cache3_size;
	int bitmap_cache_persist_enable;
	int bitmap_cache_version;
	/* font */
	int char_stamp;
	xrdpCharItem char_items[12][256];
	/* pointer */
	int pointer_stamp;
	xrdpPointerItem pointer_items[32];
	int pointer_cache_entries;
	int brush_stamp;
	xrdpBrushItem brush_items[64];
	xrdpOffscreenBitmapItem os_bitmap_items[2000];
	xrdpList* xrdp_os_del_list;
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
	xrdpProcess* pro_layer; /* owner */
	xrdpBitmap* screen;
	xrdpSession* session;
	xrdpPainter* painter;
	xrdpCache* cache;
	int palette[256];
	xrdpBitmap* login_window;
	/* generic colors */
	int black;
	int grey;
	int dark_grey;
	int blue;
	int dark_blue;
	int white;
	int red;
	int green;
	int background;
	/* dragging info */
	int dragging;
	int draggingx;
	int draggingy;
	int draggingcx;
	int draggingcy;
	int draggingdx;
	int draggingdy;
	int draggingorgx;
	int draggingorgy;
	int draggingxorstate;
	xrdpBitmap* dragging_window;
	/* the down(clicked) button */
	xrdpBitmap* button_down;
	/* popup for combo box */
	xrdpBitmap* popup_wnd;
	/* focused window */
	xrdpBitmap* focused_window;
	/* pointer */
	int current_pointer;
	int mouse_x;
	int mouse_y;
	/* keyboard info */
	int keys[256]; /* key states 0 up 1 down*/
	int caps_lock;
	int scroll_lock;
	int num_lock;
	/* session log */
	xrdpList* log;
	xrdpBitmap* log_wnd;
	int login_mode;
	HANDLE LoginModeEvent;
	xrdpMm* mm;
	xrdpFont* default_font;
	xrdpKeymap keymap;
	int hide_log_window;
	xrdpBitmap* target_surface; /* either screen or os surface */
	int current_surface_index;
	int hints;
	int allowedchannels[MAX_NR_CHANNELS];
	int allowedinitialized;
	char pamerrortxt[256];
};

/* region */
struct xrdp_region
{
	xrdpWm* wm; /* owner */
	xrdpList* rects;
};

/* painter */
struct xrdp_painter
{
	int rop;
	xrdpRect* use_clip; /* nil if not using clip */
	xrdpRect clip;
	int clip_children;
	int bg_color;
	int fg_color;
	xrdpBrush brush;
	xrdpPen pen;
	xrdpSession* session;
	xrdpWm* wm; /* owner */
	xrdpFont* font;
};

/* window or bitmap */
struct xrdp_bitmap
{
	/* 0 = bitmap 1 = window 2 = screen 3 = button 4 = image 5 = edit
	 6 = label 7 = combo 8 = special */
	int type;
	int width;
	int height;
	xrdpWm* wm;
	/* msg 1 = click 2 = mouse move 3 = paint 100 = modal result */
	/* see messages in constants.h */
	int (*notify)(xrdpBitmap* wnd, xrdpBitmap* sender, int msg, long param1, long param2);
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
	char* caption1;
	/* for window or screen */
	xrdpBitmap* modal_dialog;
	xrdpBitmap* focused_control;
	xrdpBitmap* owner; /* window that created us */
	xrdpBitmap* parent; /* window contained in */
	/* for modal dialog */
	xrdpBitmap* default_button; /* button when enter is pressed */
	xrdpBitmap* esc_button; /* button when esc is pressed */
	/* list of child windows */
	xrdpList* child_list;
	/* for edit */
	int edit_pos;
	wchar_t password_char;
	/* for button or combo */
	int state; /* for button 0 = normal 1 = down */
	/* for combo */
	xrdpList* string_list;
	xrdpList* data_list;
	/* for combo or popup */
	int item_index;
	/* for popup */
	xrdpBitmap* popped_from;
	int item_height;
	/* crc */
	int crc;
};

#define NUM_FONTS 0x4e00
#define DEFAULT_FONT_NAME "sans-10.fv1"

#define DEFAULT_ELEMENT_TOP   35
#define DEFAULT_BUTTON_W      60
#define DEFAULT_BUTTON_H      23
#define DEFAULT_COMBO_W       210
#define DEFAULT_COMBO_H       21
#define DEFAULT_EDIT_W        210
#define DEFAULT_EDIT_H        21
#define DEFAULT_WND_LOGIN_W   500
#define DEFAULT_WND_LOGIN_H   250
#define DEFAULT_WND_HELP_W    340
#define DEFAULT_WND_HELP_H    300
#define DEFAULT_WND_LOG_W     400
#define DEFAULT_WND_LOG_H     400
#define DEFAULT_WND_SPECIAL_H 100

/* font */
struct xrdp_font
{
	xrdpWm* wm;
	xrdpFontChar font_items[NUM_FONTS];
	char name[32];
	int size;
	int style;
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
#define WND_TYPE_BITMAP  0
#define WND_TYPE_WND     1
#define WND_TYPE_SCREEN  2
#define WND_TYPE_BUTTON  3
#define WND_TYPE_IMAGE   4
#define WND_TYPE_EDIT    5
#define WND_TYPE_LABEL   6
#define WND_TYPE_COMBO   7
#define WND_TYPE_SPECIAL 8
#define WND_TYPE_LISTBOX 9
#define WND_TYPE_OFFSCREEN 10

/* button states */
#define BUTTON_STATE_UP   0
#define BUTTON_STATE_DOWN 1

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

#define CB_ITEMCHANGE  300

/* xrdp.c */
long g_xrdp_sync(long (*sync_func)(long param1, long param2), long sync_param1, long sync_param2);
int g_is_term(void);
void g_set_term(int in_val);
HANDLE g_get_term_event(void);
HANDLE g_get_sync_event(void);
void g_process_waiting_function(void);

/* xrdp_cache.c */
xrdpCache* xrdp_cache_create(xrdpWm* owner, xrdpSession* session);
void xrdp_cache_delete(xrdpCache* self);
int xrdp_cache_reset(xrdpCache* self);
int xrdp_cache_add_bitmap(xrdpCache* self, xrdpBitmap* bitmap, int hints);
int xrdp_cache_add_palette(xrdpCache* self, int* palette);
int xrdp_cache_add_char(xrdpCache* self, xrdpFontChar* font_item);
int xrdp_cache_add_pointer(xrdpCache* self, xrdpPointerItem* pointer_item);
int xrdp_cache_add_pointer_static(xrdpCache* self, xrdpPointerItem* pointer_item, int index);
int xrdp_cache_add_brush(xrdpCache* self, char* brush_item_data);
int xrdp_cache_add_os_bitmap(xrdpCache* self, xrdpBitmap* bitmap, int rdpindex);
int xrdp_cache_remove_os_bitmap(xrdpCache* self, int rdpindex);
struct xrdp_os_bitmap_item* xrdp_cache_get_os_bitmap(xrdpCache* self, int rdpindex);

/* xrdp_wm.c */
xrdpWm* xrdp_wm_create(xrdpProcess* owner);
void xrdp_wm_delete(xrdpWm* self);
int xrdp_wm_send_palette(xrdpWm* self);
int xrdp_wm_send_bell(xrdpWm* self);
int xrdp_wm_load_static_colors_plus(xrdpWm* self, char* autorun_name);
int xrdp_wm_load_static_pointers(xrdpWm* self);
int xrdp_wm_init(xrdpWm* self);
int xrdp_wm_send_bitmap(xrdpWm* self, xrdpBitmap* bitmap,
		int x, int y, int cx, int cy);
int xrdp_wm_set_pointer(xrdpWm* self, int cache_idx);
int xrdp_wm_set_focused(xrdpWm* self, xrdpBitmap* wnd);
int xrdp_wm_get_vis_region(xrdpWm* self, xrdpBitmap* bitmap,
		int x, int y, int cx, int cy,
		xrdpRegion* region, int clip_children);
int xrdp_wm_mouse_move(xrdpWm* self, int x, int y);
int xrdp_wm_mouse_click(xrdpWm* self, int x, int y, int but, int down);
int xrdp_wm_process_input_mouse(xrdpWm *self, int device_flags, int x, int y);
int xrdp_wm_key(xrdpWm* self, int device_flags, int scan_code);
int xrdp_wm_key_sync(xrdpWm* self, int device_flags, int key_flags);
int xrdp_wm_pu(xrdpWm* self, xrdpBitmap* control);
int xrdp_wm_send_pointer(xrdpWm* self, int cache_idx, char* data, char* mask, int x, int y, int bpp);
int xrdp_wm_pointer(xrdpWm* self, char* data, char* mask, int x, int y, int bpp);
int callback(long id, int msg, long param1, long param2, long param3, long param4);
int xrdp_wm_delete_all_childs(xrdpWm* self);
int xrdp_wm_log_msg(xrdpWm* self, char* msg);
int xrdp_wm_get_event_handles(xrdpWm* self, HANDLE* events, DWORD* nCount);
int xrdp_wm_check_wait_objs(xrdpWm* self);
int xrdp_wm_set_login_mode(xrdpWm* self, int login_mode);

/* xrdp_process.c */
xrdpProcess* xrdp_process_create_ex(xrdpListener* owner, HANDLE DoneEvent, void* transport);
void xrdp_process_delete(xrdpProcess* self);
int xrdp_process_get_status(xrdpProcess* self);
HANDLE xrdp_process_get_term_event(xrdpProcess* self);
xrdpSession* xrdp_process_get_session(xrdpProcess* self);
int xrdp_process_get_session_id(xrdpProcess* self);
xrdpWm* xrdp_process_get_wm(xrdpProcess* self);
void* xrdp_process_main_thread(void* arg);

/* xrdp_listen.c */
xrdpListener* xrdp_listen_create(void);
void xrdp_listen_delete(xrdpListener* self);
int xrdp_listen_main_loop(xrdpListener* self);

/* xrdp_region.c */
xrdpRegion* xrdp_region_create(xrdpWm* wm);
void xrdp_region_delete(xrdpRegion* self);
int xrdp_region_add_rect(xrdpRegion* self, xrdpRect* rect);
int xrdp_region_insert_rect(xrdpRegion* self, int i, int left,
		int top, int right, int bottom);
int xrdp_region_subtract_rect(xrdpRegion* self, xrdpRect* rect);
int xrdp_region_get_rect(xrdpRegion* self, int index, xrdpRect* rect);

/* xrdp_bitmap.c */
xrdpBitmap* xrdp_bitmap_create(int width, int height, int bpp, int type, xrdpWm* wm);
xrdpBitmap* xrdp_bitmap_create_with_data(int width, int height, int bpp, char* data, xrdpWm* wm);
void xrdp_bitmap_delete(xrdpBitmap* self);
xrdpBitmap* xrdp_bitmap_get_child_by_id(xrdpBitmap* self, int id);
int xrdp_bitmap_set_focus(xrdpBitmap* self, int focused);
int xrdp_bitmap_resize(xrdpBitmap* self, int width, int height);
int xrdp_bitmap_load(xrdpBitmap* self, const char* filename, int* palette);
int xrdp_bitmap_get_pixel(xrdpBitmap* self, int x, int y);
int xrdp_bitmap_set_pixel(xrdpBitmap* self, int x, int y, int pixel);
int xrdp_bitmap_copy_box(xrdpBitmap* self,
		xrdpBitmap* dest, int x, int y, int cx, int cy);
int xrdp_bitmap_copy_box_with_crc(xrdpBitmap* self,
		xrdpBitmap* dest, int x, int y, int cx, int cy);
int xrdp_bitmap_compare(xrdpBitmap* self, xrdpBitmap* b);
int xrdp_bitmap_compare_with_crc(xrdpBitmap* self, xrdpBitmap* b);
int xrdp_bitmap_invalidate(xrdpBitmap* self, xrdpRect* rect);
int xrdp_bitmap_def_proc(xrdpBitmap* self, int msg, int param1, int param2);
int xrdp_bitmap_to_screenx(xrdpBitmap* self, int x);
int xrdp_bitmap_to_screeny(xrdpBitmap* self, int y);
int xrdp_bitmap_from_screenx(xrdpBitmap* self, int x);
int xrdp_bitmap_from_screeny(xrdpBitmap* self, int y);
int xrdp_bitmap_get_screen_clip(xrdpBitmap* self,
		xrdpPainter* painter, xrdpRect* rect, int* dx, int* dy);

/* xrdp_painter.c */
xrdpPainter* xrdp_painter_create(xrdpWm* wm, xrdpSession* session);
void xrdp_painter_delete(xrdpPainter* self);
int wm_painter_set_target(xrdpPainter* self);
int xrdp_painter_begin_update(xrdpPainter* self);
int xrdp_painter_end_update(xrdpPainter* self);
int xrdp_painter_font_needed(xrdpPainter* self);
int xrdp_painter_set_clip(xrdpPainter* self, int x, int y, int cx, int cy);
int xrdp_painter_clr_clip(xrdpPainter* self);
int xrdp_painter_fill_rect(xrdpPainter* self, xrdpBitmap* bitmap, int x, int y, int cx, int cy);
int xrdp_painter_patblt(xrdpPainter *self, xrdpBitmap* dst, int x, int y, int cx, int cy);
int xrdp_painter_draw_bitmap(xrdpPainter* self, xrdpBitmap* bitmap, xrdpBitmap* to_draw, int x, int y, int cx, int cy);
int xrdp_painter_text_width(xrdpPainter* self, char* text);
int xrdp_painter_text_height(xrdpPainter* self, char* text);
int xrdp_painter_draw_text(xrdpPainter* self, xrdpBitmap* bitmap, int x, int y, const char* text);
int xrdp_painter_draw_text2(xrdpPainter *self, xrdpBitmap* dst, GLYPH_INDEX_ORDER* msg);
int xrdp_painter_copy(xrdpPainter* self, xrdpBitmap* src, xrdpBitmap* dst,
		int x, int y, int cx, int cy, int srcx, int srcy);
int xrdp_painter_line(xrdpPainter* self, xrdpBitmap* bitmap, int x1, int y1, int x2, int y2);

/* xrdp_font.c */
xrdpFont* xrdp_font_create(xrdpWm* wm);
void xrdp_font_delete(xrdpFont* self);
int xrdp_font_item_compare(xrdpFontChar* font1,
		xrdpFontChar* font2);

/* funcs.c */
int rect_contains_pt(xrdpRect* in, int x, int y);
int rect_intersect(xrdpRect* in1, xrdpRect* in2,
		xrdpRect* out);
int rect_contained_by(xrdpRect* in1, int left, int top,
		int right, int bottom);
int check_bounds(xrdpBitmap* b, int* x, int* y, int* cx, int* cy);
int add_char_at(char* text, int text_size, wchar_t ch, int index);
int remove_char_at(char* text, int text_size, int index);
int set_string(char** in_str, const char* in);
int wchar_repeat(wchar_t* dest, int dest_size_in_wchars, wchar_t ch, int repeat);

/* in lang.c */
struct xrdp_key_info* get_key_info_from_scan_code(int device_flags, int scan_code, int* keys,
		int caps_lock, int num_lock, int scroll_lock,
		xrdpKeymap* keymap);
int get_keysym_from_scan_code(int device_flags, int scan_code, int* keys,
		int caps_lock, int num_lock, int scroll_lock,
		xrdpKeymap* keymap);
wchar_t get_char_from_scan_code(int device_flags, int scan_code, int* keys,
		int caps_lock, int num_lock, int scroll_lock,
		xrdpKeymap* keymap);
int get_keymaps(int keylayout, xrdpKeymap* keymap);

/* xrdp_login_wnd.c */
int xrdp_login_wnd_create(xrdpWm* self);

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

/**
 * Module Interface
 */

typedef int (*pXrdpClientStart)(xrdpModule* mod, int width, int height, int bpp);
typedef int (*pXrdpClientConnect)(xrdpModule* mod);
typedef int (*pXrdpClientEvent)(xrdpModule* mod, int msg, long param1, long param2, long param3, long param4);
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
	pXrdpClientEnd End;
	pXrdpClientSetParam SetParam;
	pXrdpClientSessionChange SessionChange;
	pXrdpClientGetEventHandles GetEventHandles;
	pXrdpClientCheckEventHandles CheckEventHandles;
};
typedef struct xrdp_client_module xrdpClientModule;

typedef int (*pXrdpServerBeginUpdate)(xrdpModule* mod);
typedef int (*pXrdpServerEndUpdate)(xrdpModule* mod);
typedef int (*pXrdpServerBeep)(xrdpModule* mod);
typedef int (*pXrdpServerMessage)(xrdpModule* mod, char* msg, int code);
typedef int (*pXrdpServerIsTerminated)(xrdpModule* mod);
typedef int (*pXrdpServerOpaqueRect)(xrdpModule* mod, XRDP_MSG_OPAQUE_RECT* msg);
typedef int (*pXrdpServerScreenBlt)(xrdpModule* mod, XRDP_MSG_SCREEN_BLT* msg);
typedef int (*pXrdpServerPaintRect)(xrdpModule* mod, XRDP_MSG_PAINT_RECT* msg);
typedef int (*pXrdpServerPatBlt)(xrdpModule* mod, XRDP_MSG_PATBLT* msg);
typedef int (*pXrdpServerSetPointer)(xrdpModule* mod, XRDP_MSG_SET_POINTER* msg);
typedef int (*pXrdpServerSetPalette)(xrdpModule* mod, int* palette);
typedef int (*pXrdpServerSetClippingRegion)(xrdpModule* mod, XRDP_MSG_SET_CLIPPING_REGION* msg);
typedef int (*pXrdpServerSetNullClippingRegion)(xrdpModule* mod);
typedef int (*pXrdpServerSetForeColor)(xrdpModule* mod, int fgcolor);
typedef int (*pXrdpServerSetBackColor)(xrdpModule* mod, int bgcolor);
typedef int (*pXrdpServerSetRop2)(xrdpModule* mod, int opcode);
typedef int (*pXrdpServerSetPen)(xrdpModule* mod, int style, int width);
typedef int (*pXrdpServerLineTo)(xrdpModule* mod, int x1, int y1, int x2, int y2);
typedef int (*pXrdpServerAddChar)(xrdpModule* mod, XRDP_MSG_CACHE_GLYPH* msg);
typedef int (*pXrdpServerText)(xrdpModule* mod, GLYPH_INDEX_ORDER* msg);
typedef int (*pXrdpServerReset)(xrdpModule* mod, int width, int height, int bpp);
typedef int (*pXrdpServerCreateOffscreenSurface)(xrdpModule* mod, int id, int width, int height);
typedef int (*pXrdpServerSwitchOffscreenSurface)(xrdpModule* mod, int id);
typedef int (*pXrdpServerDeleteOffscreenSurface)(xrdpModule* mod, int id);
typedef int (*pXrdpServerPaintOffscreenRect)(xrdpModule* mod, int x, int y, int cx, int cy, int id, int srcx, int srcy);
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
	pXrdpServerMessage Message;
	pXrdpServerIsTerminated IsTerminated;
	pXrdpServerOpaqueRect OpaqueRect;
	pXrdpServerScreenBlt ScreenBlt;
	pXrdpServerPaintRect PaintRect;
	pXrdpServerPatBlt PatBlt;
	pXrdpServerSetPointer SetPointer;
	pXrdpServerSetPalette SetPalette;
	pXrdpServerSetClippingRegion SetClippingRegion;
	pXrdpServerSetNullClippingRegion SetNullClippingRegion;
	pXrdpServerSetForeColor SetForeColor;
	pXrdpServerSetBackColor SetBackColor;
	pXrdpServerSetRop2 SetRop2;
	pXrdpServerSetPen SetPen;
	pXrdpServerLineTo LineTo;
	pXrdpServerAddChar AddChar;
	pXrdpServerText Text;
	pXrdpServerReset Reset;
	pXrdpServerCreateOffscreenSurface CreateOffscreenSurface;
	pXrdpServerSwitchOffscreenSurface SwitchOffscreenSurface;
	pXrdpServerDeleteOffscreenSurface DeleteOffscreenSurface;
	pXrdpServerPaintOffscreenRect PaintOffscreenRect;
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
};

#endif /* XRDP_H */
