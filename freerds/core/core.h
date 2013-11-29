/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP X11 Server
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

#ifndef FREERDP_RDS_NG_CORE_H
#define FREERDP_RDS_NG_CORE_H

#include <freerdp/api.h>
#include <freerdp/freerdp.h>
#include <freerdp/codec/rfx.h>
#include <freerdp/codec/nsc.h>

#include <freerdp/channels/wtsvc.h>
#include <freerdp/server/cliprdr.h>
#include <freerdp/server/rdpdr.h>
#include <freerdp/server/rdpsnd.h>
#include <freerdp/server/drdynvc.h>

#include <winpr/crt.h>
#include <winpr/stream.h>
#include <freerds/backend.h>


typedef struct rds_backend_connector rdsBackendConnector;

struct rds_rect
{
	int left;
	int top;
	int right;
	int bottom;
};
typedef struct rds_rect rdsRect;

struct rds_connection
{
	rdpContext context;

	long id;
	rdsBackendConnector* connector;
	HANDLE Thread;
	HANDLE TermEvent;
	freerdp_peer* client;
	rdpSettings* settings;

	BOOL codecMode;
	int bytesPerPixel;

	wStream* bs;
	wStream* bts;

	wStream* rfx_s;
	RFX_CONTEXT* rfx_context;

	wStream* nsc_s;
	NSC_CONTEXT* nsc_context;

	UINT32 frameId;
	wListDictionary* FrameList;

	WTSVirtualChannelManager* vcm;
	CliprdrServerContext* cliprdr;
	RdpdrServerContext* rdpdr;
	RdpsndServerContext* rdpsnd;
	DrdynvcServerContext* drdynvc;
	wMessageQueue* notifications;
};


struct rds_backend_connector
{
	DEFINE_BACKEND_COMMON();

	int MaxFps;
	int fps;
	wLinkedList* ServerList;
	wMessageQueue* ServerQueue;
	rdsServerInterface* ServerProxy;
	freerdp* instance;
	rdpSettings* settings;
	rdsConnection* connection;
};

struct rds_notification_msg_switch
{
	UINT32 tag;
	char *endpoint;
};

struct rds_notification_msg_logoff
{
	UINT32 tag;
};

enum notifications
{
	NOTIFY_SWITCHTO = 0,
	NOTIFY_LOGOFF,
};

#ifdef __cplusplus
extern "C" {
#endif

FREERDP_API int freerds_connection_init(rdsConnection* connection, rdpSettings* settings);
FREERDP_API void freerds_connection_uninit(rdsConnection* connection);

FREERDP_API int freerds_send_palette(rdsConnection* connection, int* palette);

FREERDP_API int freerds_send_bell(rdsConnection* connection);

FREERDP_API int freerds_send_bitmap_update(rdsConnection* connection, int bpp, RDS_MSG_PAINT_RECT* msg);

FREERDP_API int freerds_set_pointer(rdsConnection* connection, RDS_MSG_SET_POINTER* msg);

FREERDP_API int freerds_set_system_pointer(rdsConnection* connection, RDS_MSG_SET_SYSTEM_POINTER* msg);

FREERDP_API int freerds_orders_begin_paint(rdsConnection* connection);

FREERDP_API int freerds_orders_end_paint(rdsConnection* connection);

FREERDP_API int freerds_orders_rect(rdsConnection* connection, int x, int y,
		int cx, int cy, int color, rdsRect* rect);

FREERDP_API int freerds_orders_screen_blt(rdsConnection* connection, int x, int y,
		int cx, int cy, int srcx, int srcy, int rop, rdsRect* rect);

FREERDP_API int freerds_orders_pat_blt(rdsConnection* connection, int x, int y,
		int cx, int cy, int rop, int bg_color, int fg_color,
		rdpBrush* brush, rdsRect* rect);

FREERDP_API int freerds_orders_dest_blt(rdsConnection* connection,
		int x, int y, int cx, int cy, int rop, rdsRect* rect);

FREERDP_API int freerds_orders_line(rdsConnection* connection, RDS_MSG_LINE_TO* msg, rdsRect* rect);

FREERDP_API int freerds_orders_mem_blt(rdsConnection* connection, int cache_id,
		int color_table, int x, int y, int cx, int cy, int rop, int srcx,
		int srcy, int cache_idx, rdsRect* rect);

FREERDP_API int freerds_orders_text(rdsConnection* connection, RDS_MSG_GLYPH_INDEX* msg, rdsRect* rect);

FREERDP_API int freerds_orders_send_palette(rdsConnection* connection, int* palette, int cache_id);

FREERDP_API int freerds_orders_send_raw_bitmap(rdsConnection* connection,
		int width, int height, int bpp, char* data,
		int cache_id, int cache_idx);

FREERDP_API int freerds_orders_send_bitmap(rdsConnection* connection,
		int width, int height, int bpp, char* data,
		int cache_id, int cache_idx);

FREERDP_API int freerds_orders_send_font(rdsConnection* connection, RDS_MSG_CACHE_GLYPH* msg);

FREERDP_API int freerds_reset(rdsConnection* connection, RDS_MSG_RESET* msg);

FREERDP_API int freerds_orders_send_raw_bitmap2(rdsConnection* connection,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx);

FREERDP_API int freerds_orders_send_bitmap2(rdsConnection* connection,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx, int hints);

FREERDP_API int freerds_orders_send_bitmap3(rdsConnection* connection,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx, int hints);

FREERDP_API int freerds_orders_send_brush(rdsConnection* connection, int width, int height,
		int bpp, int type, int size, char* data, int cache_id);

FREERDP_API int freerds_orders_send_create_os_surface(rdsConnection* connection,
		CREATE_OFFSCREEN_BITMAP_ORDER* createOffscreenBitmap);

FREERDP_API int freerds_orders_send_switch_os_surface(rdsConnection* connection, int id);

FREERDP_API int freerds_send_surface_bits(rdsConnection* connection, int bpp, RDS_MSG_PAINT_RECT* msg);

FREERDP_API int freerds_orders_send_frame_marker(rdsConnection* connection, UINT32 action, UINT32 id);

FREERDP_API int freerds_window_new_update(rdsConnection* connection, RDS_MSG_WINDOW_NEW_UPDATE* msg);

FREERDP_API int freerds_window_delete(rdsConnection* connection, RDS_MSG_WINDOW_DELETE* msg);


#ifdef __cplusplus
}
#endif


#endif /* FREERDP_RDS_NG_CORE_H */
