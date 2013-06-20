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

#include <freerdp/freerdp.h>

#include "arch.h"
#include "parse.h"
#include "trans.h"
#include "list.h"

#include "core.h"

#include "xrdp_types.h"
#include "defines.h"
#include "os_calls.h"
#include "thread_calls.h"
#include "file.h"

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
tbus g_get_term_event(void);
tbus g_get_sync_event(void);
void g_process_waiting_function(void);

/* xrdp_cache.c */
struct xrdp_cache* xrdp_cache_create(xrdpWm* owner, xrdpSession* session);
void xrdp_cache_delete(struct xrdp_cache* self);
int xrdp_cache_reset(struct xrdp_cache* self);
int xrdp_cache_add_bitmap(struct xrdp_cache* self, struct xrdp_bitmap* bitmap, int hints);
int xrdp_cache_add_palette(struct xrdp_cache* self, int* palette);
int xrdp_cache_add_char(struct xrdp_cache* self, struct xrdp_font_char* font_item);
int xrdp_cache_add_pointer(struct xrdp_cache* self, struct xrdp_pointer_item* pointer_item);
int xrdp_cache_add_pointer_static(struct xrdp_cache* self, struct xrdp_pointer_item* pointer_item, int index);
int xrdp_cache_add_brush(struct xrdp_cache* self, char* brush_item_data);
int xrdp_cache_add_os_bitmap(struct xrdp_cache* self, struct xrdp_bitmap* bitmap, int rdpindex);
int xrdp_cache_remove_os_bitmap(struct xrdp_cache* self, int rdpindex);
struct xrdp_os_bitmap_item* xrdp_cache_get_os_bitmap(struct xrdp_cache* self, int rdpindex);

/* xrdp_wm.c */
xrdpWm* xrdp_wm_create(xrdpProcess* owner);
void xrdp_wm_delete(xrdpWm* self);
int xrdp_wm_send_palette(xrdpWm* self);
int xrdp_wm_send_bell(xrdpWm* self);
int xrdp_wm_load_static_colors_plus(xrdpWm* self, char* autorun_name);
int xrdp_wm_load_static_pointers(xrdpWm* self);
int xrdp_wm_init(xrdpWm* self);
int xrdp_wm_send_bitmap(xrdpWm* self, struct xrdp_bitmap* bitmap,
		int x, int y, int cx, int cy);
int xrdp_wm_set_pointer(xrdpWm* self, int cache_idx);
int xrdp_wm_set_focused(xrdpWm* self, struct xrdp_bitmap* wnd);
int xrdp_wm_get_vis_region(xrdpWm* self, struct xrdp_bitmap* bitmap,
		int x, int y, int cx, int cy,
		struct xrdp_region* region, int clip_children);
int xrdp_wm_mouse_move(xrdpWm* self, int x, int y);
int xrdp_wm_mouse_click(xrdpWm* self, int x, int y, int but, int down);
int xrdp_wm_process_input_mouse(xrdpWm *self, int device_flags, int x, int y);
int xrdp_wm_key(xrdpWm* self, int device_flags, int scan_code);
int xrdp_wm_key_sync(xrdpWm* self, int device_flags, int key_flags);
int xrdp_wm_pu(xrdpWm* self, struct xrdp_bitmap* control);
int xrdp_wm_send_pointer(xrdpWm* self, int cache_idx,
		char* data, char* mask, int x, int y, int bpp);
int xrdp_wm_pointer(xrdpWm* self, char* data, char* mask, int x, int y,
		int bpp);
int callback(long id, int msg, long param1, long param2, long param3, long param4);
int xrdp_wm_delete_all_childs(xrdpWm* self);
int xrdp_wm_log_msg(xrdpWm* self, char* msg);
int xrdp_wm_get_wait_objs(xrdpWm* self, tbus* robjs, int* rc,
		tbus* wobjs, int* wc, int* timeout);
int xrdp_wm_check_wait_objs(xrdpWm* self);
int xrdp_wm_set_login_mode(xrdpWm* self, int login_mode);

/* xrdp_process.c */
xrdpProcess* xrdp_process_create(xrdpListener* owner, tbus done_event);
xrdpProcess* xrdp_process_create_ex(xrdpListener* owner, tbus done_event, void* transport);
void xrdp_process_delete(xrdpProcess* self);
int xrdp_process_get_status(xrdpProcess* self);
tbus xrdp_process_get_term_event(xrdpProcess* self);
xrdpSession* xrdp_process_get_session(xrdpProcess* self);
int xrdp_process_get_session_id(xrdpProcess* self);
xrdpWm* xrdp_process_get_wm(xrdpProcess* self);
void xrdp_process_set_transport(xrdpProcess* self, struct trans* transport);
int xrdp_process_main_loop(xrdpProcess* self);
void* xrdp_process_main_thread(void* arg);

/* xrdp_listen.c */
xrdpListener* xrdp_listen_create(void);
void xrdp_listen_delete(xrdpListener* self);
int xrdp_listen_main_loop(xrdpListener* self);
int xrdp_listen_set_startup_params(xrdpListener *self, struct xrdp_startup_params* startup_params);

/* xrdp_region.c */
struct xrdp_region* xrdp_region_create(xrdpWm* wm);
void xrdp_region_delete(struct xrdp_region* self);
int xrdp_region_add_rect(struct xrdp_region* self, struct xrdp_rect* rect);
int xrdp_region_insert_rect(struct xrdp_region* self, int i, int left,
		int top, int right, int bottom);
int xrdp_region_subtract_rect(struct xrdp_region* self, struct xrdp_rect* rect);
int xrdp_region_get_rect(struct xrdp_region* self, int index, struct xrdp_rect* rect);

/* xrdp_bitmap.c */
struct xrdp_bitmap* xrdp_bitmap_create(int width, int height, int bpp, int type, xrdpWm* wm);
struct xrdp_bitmap* xrdp_bitmap_create_with_data(int width, int height, int bpp, char* data, xrdpWm* wm);
void xrdp_bitmap_delete(struct xrdp_bitmap* self);
struct xrdp_bitmap* xrdp_bitmap_get_child_by_id(struct xrdp_bitmap* self, int id);
int xrdp_bitmap_set_focus(struct xrdp_bitmap* self, int focused);
int xrdp_bitmap_resize(struct xrdp_bitmap* self, int width, int height);
int xrdp_bitmap_load(struct xrdp_bitmap* self, const char* filename, int* palette);
int xrdp_bitmap_get_pixel(struct xrdp_bitmap* self, int x, int y);
int xrdp_bitmap_set_pixel(struct xrdp_bitmap* self, int x, int y, int pixel);
int xrdp_bitmap_copy_box(struct xrdp_bitmap* self,
		struct xrdp_bitmap* dest, int x, int y, int cx, int cy);
int xrdp_bitmap_copy_box_with_crc(struct xrdp_bitmap* self,
		struct xrdp_bitmap* dest, int x, int y, int cx, int cy);
int xrdp_bitmap_compare(struct xrdp_bitmap* self, struct xrdp_bitmap* b);
int xrdp_bitmap_compare_with_crc(struct xrdp_bitmap* self, struct xrdp_bitmap* b);
int xrdp_bitmap_invalidate(struct xrdp_bitmap* self, struct xrdp_rect* rect);
int xrdp_bitmap_def_proc(struct xrdp_bitmap* self, int msg, int param1, int param2);
int xrdp_bitmap_to_screenx(struct xrdp_bitmap* self, int x);
int xrdp_bitmap_to_screeny(struct xrdp_bitmap* self, int y);
int xrdp_bitmap_from_screenx(struct xrdp_bitmap* self, int x);
int xrdp_bitmap_from_screeny(struct xrdp_bitmap* self, int y);
int xrdp_bitmap_get_screen_clip(struct xrdp_bitmap* self,
		struct xrdp_painter* painter, struct xrdp_rect* rect, int* dx, int* dy);

/* xrdp_painter.c */
struct xrdp_painter* xrdp_painter_create(xrdpWm* wm, xrdpSession* session);
void xrdp_painter_delete(struct xrdp_painter* self);
int wm_painter_set_target(struct xrdp_painter* self);
int xrdp_painter_begin_update(struct xrdp_painter* self);
int xrdp_painter_end_update(struct xrdp_painter* self);
int xrdp_painter_font_needed(struct xrdp_painter* self);
int xrdp_painter_set_clip(struct xrdp_painter* self,
		int x, int y, int cx, int cy);
int xrdp_painter_clr_clip(struct xrdp_painter* self);
int xrdp_painter_fill_rect(struct xrdp_painter* self,
		struct xrdp_bitmap* bitmap, int x, int y, int cx, int cy);
int xrdp_painter_draw_bitmap(struct xrdp_painter* self,
		struct xrdp_bitmap* bitmap, struct xrdp_bitmap* to_draw,
		int x, int y, int cx, int cy);
int xrdp_painter_text_width(struct xrdp_painter* self, char* text);
int xrdp_painter_text_height(struct xrdp_painter* self, char* text);
int xrdp_painter_draw_text(struct xrdp_painter* self,
		struct xrdp_bitmap* bitmap, int x, int y, const char* text);
int xrdp_painter_draw_text2(struct xrdp_painter* self,
		struct xrdp_bitmap* bitmap,
		int font, int flags, int mixmode,
		int clip_left, int clip_top,
		int clip_right, int clip_bottom,
		int box_left, int box_top,
		int box_right, int box_bottom,
		int x, int y, char* data, int data_len);
int xrdp_painter_copy(struct xrdp_painter* self,
		struct xrdp_bitmap* src,
		struct xrdp_bitmap* dst,
		int x, int y, int cx, int cy,
		int srcx, int srcy);
int xrdp_painter_line(struct xrdp_painter* self,
		struct xrdp_bitmap* bitmap,
		int x1, int y1, int x2, int y2);

/* xrdp_font.c */
struct xrdp_font* xrdp_font_create(xrdpWm* wm);
void xrdp_font_delete(struct xrdp_font* self);
int xrdp_font_item_compare(struct xrdp_font_char* font1,
		struct xrdp_font_char* font2);

/* funcs.c */
int rect_contains_pt(struct xrdp_rect* in, int x, int y);
int rect_intersect(struct xrdp_rect* in1, struct xrdp_rect* in2,
		struct xrdp_rect* out);
int rect_contained_by(struct xrdp_rect* in1, int left, int top,
		int right, int bottom);
int check_bounds(struct xrdp_bitmap* b, int* x, int* y, int* cx, int* cy);
int add_char_at(char* text, int text_size, twchar ch, int index);
int remove_char_at(char* text, int text_size, int index);
int set_string(char** in_str, const char* in);
int wchar_repeat(twchar* dest, int dest_size_in_wchars, twchar ch, int repeat);

/* in lang.c */
struct xrdp_key_info* get_key_info_from_scan_code(int device_flags, int scan_code, int* keys,
		int caps_lock, int num_lock, int scroll_lock,
		struct xrdp_keymap* keymap);
int get_keysym_from_scan_code(int device_flags, int scan_code, int* keys,
		int caps_lock, int num_lock, int scroll_lock,
		struct xrdp_keymap* keymap);
twchar get_char_from_scan_code(int device_flags, int scan_code, int* keys,
		int caps_lock, int num_lock, int scroll_lock,
		struct xrdp_keymap* keymap);
int get_keymaps(int keylayout, struct xrdp_keymap* keymap);

/* xrdp_login_wnd.c */
int xrdp_login_wnd_create(xrdpWm* self);

/* xrdp_bitmap_compress.c */
int xrdp_bitmap_compress(char* in_data, int width, int height,
		struct stream* s, int bpp, int byte_limit,
		int start_line, struct stream* temp, int e);

/* xrdp_mm.c */
struct xrdp_mm* xrdp_mm_create(xrdpWm* owner);
void xrdp_mm_delete(struct xrdp_mm* self);
int xrdp_mm_connect(struct xrdp_mm* self);
int xrdp_mm_process_channel_data(struct xrdp_mm* self, tbus param1, tbus param2,
		tbus param3, tbus param4);
int xrdp_mm_get_wait_objs(struct xrdp_mm* self,
		tbus* read_objs, int* rcount,
		tbus* write_objs, int* wcount, int* timeout);
int xrdp_mm_check_wait_objs(struct xrdp_mm* self);
int server_begin_update(struct xrdp_mod* mod);
int server_end_update(struct xrdp_mod* mod);
int server_bell_trigger(struct xrdp_mod* mod);
int server_fill_rect(struct xrdp_mod* mod, int x, int y, int cx, int cy);
int server_screen_blt(struct xrdp_mod* mod, int x, int y, int cx, int cy, int srcx, int srcy);
int server_paint_rect(struct xrdp_mod* mod, int x, int y, int cx, int cy,
		char* data, int width, int height, int srcx, int srcy);
int server_set_pointer(struct xrdp_mod* mod, int x, int y, char* data, char* mask);
int server_set_pointer_ex(struct xrdp_mod* mod, int x, int y, char* data, char* mask, int bpp);
int server_palette(struct xrdp_mod* mod, int* palette);
int server_msg(struct xrdp_mod* mod, char* msg, int code);
int server_is_term(struct xrdp_mod* mod);
int xrdp_child_fork(void);
int server_set_clip(struct xrdp_mod* mod, int x, int y, int cx, int cy);
int server_reset_clip(struct xrdp_mod* mod);
int server_set_fgcolor(struct xrdp_mod* mod, int fgcolor);
int server_set_bgcolor(struct xrdp_mod* mod, int bgcolor);
int server_set_opcode(struct xrdp_mod* mod, int opcode);
int server_set_mixmode(struct xrdp_mod* mod, int mixmode);
int server_set_brush(struct xrdp_mod* mod, int x_orgin, int y_orgin, int style, char* pattern);
int server_set_pen(struct xrdp_mod* mod, int style, int width);
int server_draw_line(struct xrdp_mod* mod, int x1, int y1, int x2, int y2);
int server_add_char(struct xrdp_mod* mod, int font, int charactor,
		int offset, int baseline, int width, int height, char* data);
int server_draw_text(struct xrdp_mod* mod, int font,
		int flags, int mixmode, int clip_left, int clip_top,
		int clip_right, int clip_bottom,
		int box_left, int box_top,
		int box_right, int box_bottom,
		int x, int y, char* data, int data_len);
int server_reset(struct xrdp_mod* mod, int width, int height, int bpp);
int is_channel_allowed(xrdpWm* wm, int channel_id);
int server_query_channel(struct xrdp_mod* mod, int index, char* channel_name, int* channel_flags);
int server_get_channel_id(struct xrdp_mod* mod, char* name);
int server_send_to_channel(struct xrdp_mod* mod, int channel_id,
		char* data, int data_len, int total_data_len, int flags);
int server_create_os_surface(struct xrdp_mod* mod, int id, int width, int height);
int server_switch_os_surface(struct xrdp_mod* mod, int id);
int server_delete_os_surface(struct xrdp_mod* mod, int id);
int server_paint_rect_os(struct xrdp_mod* mod, int x, int y, int cx, int cy, int id, int srcx, int srcy);
int server_set_hints(struct xrdp_mod* mod, int hints, int mask);
int server_window_new_update(struct xrdp_mod* mod, int window_id,
		struct rail_window_state_order* window_state, int flags);
int server_window_delete(struct xrdp_mod* mod, int window_id);
int server_window_icon(struct xrdp_mod* mod, int window_id, int cache_entry,
		int cache_id, struct rail_icon_info* icon_info, int flags);
int server_window_cached_icon(struct xrdp_mod* mod,
		int window_id, int cache_entry, int cache_id, int flags);
int server_notify_new_update(struct xrdp_mod* mod,
		int window_id, int notify_id,
		struct rail_notify_state_order* notify_state, int flags);
int server_notify_delete(struct xrdp_mod* mod, int window_id, int notify_id);
int server_monitored_desktop(struct xrdp_mod* mod,
		struct rail_monitored_desktop_order* mdo, int flags);
