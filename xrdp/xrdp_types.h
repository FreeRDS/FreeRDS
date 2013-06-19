/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2013
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
 * types
 */

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

#include "xrdp_rail.h"

#define MAX_NR_CHANNELS 16
#define MAX_CHANNEL_NAME 16

/* lib */
struct xrdp_mod
{
	int size; /* size of this struct */
	int version; /* internal version */
	/* client functions */
	int (*mod_start)(xrdpModule* v, int w, int h, int bpp);
	int (*mod_connect)(xrdpModule* v);
	int (*mod_event)(xrdpModule* v, int msg, long param1, long param2, long param3, long param4);
	int (*mod_signal)(xrdpModule* v);
	int (*mod_end)(xrdpModule* v);
	int (*mod_set_param)(xrdpModule* v, char* name, char* value);
	int (*mod_session_change)(xrdpModule* v, int, int);
	int (*mod_get_wait_objs)(xrdpModule* v, tbus* read_objs, int* rcount, tbus* write_objs, int* wcount,
			int* timeout);
	int (*mod_check_wait_objs)(xrdpModule* v);
	long mod_dumby[100 - 9]; /* align, 100 minus the number of mod
	 functions above */
	/* server functions */
	int (*server_begin_update)(xrdpModule* v);
	int (*server_end_update)(xrdpModule* v);
	int (*server_fill_rect)(xrdpModule* v, int x, int y, int cx, int cy);
	int (*server_screen_blt)(xrdpModule* v, int x, int y, int cx, int cy, int srcx, int srcy);
	int (*server_paint_rect)(xrdpModule* v, int x, int y, int cx, int cy, char* data, int width, int height,
			int srcx, int srcy);
	int (*server_set_pointer)(xrdpModule* v, int x, int y, char* data, char* mask);
	int (*server_palette)(xrdpModule* v, int* palette);
	int (*server_msg)(xrdpModule* v, char* msg, int code);
	int (*server_is_term)(xrdpModule* v);
	int (*server_set_clip)(xrdpModule* v, int x, int y, int cx, int cy);
	int (*server_reset_clip)(xrdpModule* v);
	int (*server_set_fgcolor)(xrdpModule* v, int fgcolor);
	int (*server_set_bgcolor)(xrdpModule* v, int bgcolor);
	int (*server_set_opcode)(xrdpModule* v, int opcode);
	int (*server_set_mixmode)(xrdpModule* v, int mixmode);
	int (*server_set_brush)(xrdpModule* v, int x_orgin, int y_orgin, int style, char* pattern);
	int (*server_set_pen)(xrdpModule* v, int style, int width);
	int (*server_draw_line)(xrdpModule* v, int x1, int y1, int x2, int y2);
	int (*server_add_char)(xrdpModule* v, int font, int charactor, int offset, int baseline, int width,
			int height, char* data);
	int (*server_draw_text)(xrdpModule* v, int font, int flags, int mixmode, int clip_left, int clip_top,
			int clip_right, int clip_bottom, int box_left, int box_top, int box_right, int box_bottom,
			int x, int y, char* data, int data_len);
	int (*server_reset)(xrdpModule* v, int width, int height, int bpp);
	int (*server_query_channel)(xrdpModule* v, int index, char* channel_name, int* channel_flags);
	int (*server_get_channel_id)(xrdpModule* v, char* name);
	int (*server_send_to_channel)(xrdpModule* v, int channel_id, char* data, int data_len, int total_data_len,
			int flags);
	int (*server_bell_trigger)(xrdpModule* v);
	/* off screen bitmaps */
	int (*server_create_os_surface)(xrdpModule* v, int rdpindex, int width, int height);
	int (*server_switch_os_surface)(xrdpModule* v, int rdpindex);
	int (*server_delete_os_surface)(xrdpModule* v, int rdpindex);
	int (*server_paint_rect_os)(xrdpModule* mod, int x, int y, int cx, int cy, int rdpindex, int srcx,
			int srcy);
	int (*server_set_hints)(xrdpModule* mod, int hints, int mask);
	/* rail */
	int (*server_window_new_update)(xrdpModule* mod, int window_id,
			struct rail_window_state_order* window_state, int flags);
	int (*server_window_delete)(xrdpModule* mod, int window_id);
	int (*server_window_icon)(xrdpModule* mod, int window_id, int cache_entry, int cache_id,
			struct rail_icon_info* icon_info, int flags);
	int (*server_window_cached_icon)(xrdpModule* mod, int window_id, int cache_entry, int cache_id, int flags);
	int (*server_notify_new_update)(xrdpModule* mod, int window_id, int notify_id,
			struct rail_notify_state_order* notify_state, int flags);
	int (*server_notify_delete)(xrdpModule* mod, int window_id, int notify_id);
	int (*server_monitored_desktop)(xrdpModule* mod, struct rail_monitored_desktop_order* mdo, int flags);
	int (*server_set_pointer_ex)(xrdpModule* v, int x, int y, char* data, char* mask, int bpp);

	long server_dumby[100 - 38]; /* align, 100 minus the number of server
	 functions above */
	/* common */
	long handle; /* pointer to self as int */
	long wm; /* xrdpWm* */
	long painter;
	int sck;
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

/* differnce caches */
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
	long mod_handle; /* returned from g_load_library */
	xrdpModule* (*mod_init)(void);
	int (*mod_exit)(xrdpModule*);
	xrdpModule* mod; /* module interface */
	int display; /* 10 for :10.0, 11 for :11.0, etc */
	int code; /* 0 Xvnc session 10 X11rdp session */
	int sesman_controlled; /* true if this is a sesman session */
	struct trans* chan_trans; /* connection to chansrv */
	int chan_trans_up; /* true once connected to chansrv */
	int delete_chan_trans; /* boolean set when done with channel connection */
	int usechansrv; /* true if chansrvport is set in xrdp.ini or using sesman */
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
	/* client info */
	xrdpClientInfo* client_info;
	/* session log */
	xrdpList* log;
	xrdpBitmap* log_wnd;
	int login_mode;
	tbus login_mode_event;
	struct xrdp_mm* mm;
	struct xrdp_font* default_font;
	struct xrdp_keymap keymap;
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
	int mix_mode;
	xrdpBrush brush;
	xrdpPen pen;
	xrdpSession* session;
	xrdpWm* wm; /* owner */
	struct xrdp_font* font;
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
	twchar password_char;
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
