/*
Copyright 2005-2013 Jay Sorg

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#include "rdp.h"
#include "xrdp_rail.h"

#include <avro.h>

#include <xrdp-ng/xrdp.h>

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
		do { if (_level < LOG_LEVEL) { ErrorF _args ; } } while (0)
#define LLOGLN(_level, _args) \
		do { if (_level < LOG_LEVEL) { ErrorF _args ; ErrorF("\n"); } } while (0)

static int g_listen_sck = 0;
static int g_sck = 0;
static int g_sck_closed = 0;
static int g_connected = 0;
static int g_dis_listen_sck = 0;

static int g_begin = 0;
static wStream* g_out_s = 0;
static wStream* g_in_s = 0;
static int g_button_mask = 0;
static int g_cursor_x = 0;
static int g_cursor_y = 0;
static OsTimerPtr g_timer = 0;
static int g_scheduled = 0;
static int g_count = 0;
static int g_rdpindex = -1;

static BYTE* pfbBackBufferMemory = NULL;

extern DevPrivateKeyRec g_rdpWindowIndex; /* from rdpmain.c */
extern ScreenPtr g_pScreen; /* from rdpmain.c */
extern int g_Bpp; /* from rdpmain.c */
extern int g_Bpp_mask; /* from rdpmain.c */
extern rdpScreenInfoRec g_rdpScreen; /* from rdpmain.c */
extern int g_can_do_pix_to_pix; /* from rdpmain.c */
extern int g_use_rail; /* from rdpmain.c */

/* true is to use unix domain socket */
extern int g_use_uds; /* in rdpmain.c */
extern char g_uds_data[]; /* in rdpmain.c */
extern int g_do_dirty_ons; /* in rdpmain.c */
extern rdpPixmapRec g_screenPriv; /* in rdpmain.c */
extern int g_con_number; /* in rdpmain.c */

struct rdpup_os_bitmap
{
	int used;
	PixmapPtr pixmap;
	rdpPixmapPtr priv;
	int stamp;
};

static struct rdpup_os_bitmap *g_os_bitmaps = 0;
static int g_max_os_bitmaps = 0;
static int g_os_bitmap_stamp = 0;

static int g_pixmap_byte_total = 0;
static int g_pixmap_num_used = 0;

struct rdpup_top_window
{
	WindowPtr wnd;
	struct rdpup_top_window *next;
};

/*
0 GXclear,        0
1 GXnor,          DPon
2 GXandInverted,  DPna
3 GXcopyInverted, Pn
4 GXandReverse,   PDna
5 GXinvert,       Dn
6 GXxor,          DPx
7 GXnand,         DPan
8 GXand,          DPa
9 GXequiv,        DPxn
a GXnoop,         D
b GXorInverted,   DPno
c GXcopy,         P
d GXorReverse,   PDno
e GXor,          DPo
f GXset          1
 */

static int g_rdp_opcodes[16] =
{
		0x00, /* GXclear        0x0 0 */
		0x88, /* GXand          0x1 src AND dst */
		0x44, /* GXandReverse   0x2 src AND NOT dst */
		0xcc, /* GXcopy         0x3 src */
		0x22, /* GXandInverted  0x4 NOT src AND dst */
		0xaa, /* GXnoop         0x5 dst */
		0x66, /* GXxor          0x6 src XOR dst */
		0xee, /* GXor           0x7 src OR dst */
		0x11, /* GXnor          0x8 NOT src AND NOT dst */
		0x99, /* GXequiv        0x9 NOT src XOR dst */
		0x55, /* GXinvert       0xa NOT dst */
		0xdd, /* GXorReverse    0xb src OR NOT dst */
		0x33, /* GXcopyInverted 0xc NOT src */
		0xbb, /* GXorInverted   0xd NOT src OR dst */
		0x77, /* GXnand         0xe NOT src OR NOT dst */
		0xff  /* GXset          0xf 1 */
};

#define COLOR8(r, g, b) \
		((((r) >> 5) << 0)  | (((g) >> 5) << 3) | (((b) >> 6) << 6))
#define COLOR15(r, g, b) \
		((((r) >> 3) << 10) | (((g) >> 3) << 5) | (((b) >> 3) << 0))
#define COLOR16(r, g, b) \
		((((r) >> 3) << 11) | (((g) >> 2) << 5) | (((b) >> 3) << 0))
#define COLOR24(r, g, b) \
		((((r) >> 0) << 0)  | (((g) >> 0) << 8) | (((b) >> 0) << 16))
#define SPLITCOLOR32(r, g, b, c) \
		{ \
	r = ((c) >> 16) & 0xff; \
	g = ((c) >> 8) & 0xff; \
	b = (c) & 0xff; \
		}

int convert_pixel(int in_pixel)
{
	int red;
	int green;
	int blue;
	int rv;

	rv = 0;

	if (g_rdpScreen.depth == 24)
	{
		if (g_rdpScreen.rdp_bpp == 32)
		{
			rv = in_pixel;
			SPLITCOLOR32(red, green, blue, rv);
			rv = COLOR24(red, green, blue);
		}
		else if (g_rdpScreen.rdp_bpp == 24)
		{
			rv = in_pixel;
			SPLITCOLOR32(red, green, blue, rv);
			rv = COLOR24(red, green, blue);
		}
		else if (g_rdpScreen.rdp_bpp == 16)
		{
			rv = in_pixel;
			SPLITCOLOR32(red, green, blue, rv);
			rv = COLOR16(red, green, blue);
		}
		else if (g_rdpScreen.rdp_bpp == 15)
		{
			rv = in_pixel;
			SPLITCOLOR32(red, green, blue, rv);
			rv = COLOR15(red, green, blue);
		}
		else if (g_rdpScreen.rdp_bpp == 8)
		{
			rv = in_pixel;
			SPLITCOLOR32(red, green, blue, rv);
			rv = COLOR8(red, green, blue);
		}
	}
	else if (g_rdpScreen.depth == g_rdpScreen.rdp_bpp)
	{
		return in_pixel;
	}

	return rv;
}

int convert_pixels(void *src, void *dst, int num_pixels)
{
	unsigned int pixel;
	unsigned int red;
	unsigned int green;
	unsigned int blue;
	unsigned int *src32;
	unsigned int *dst32;
	unsigned short *dst16;
	unsigned char *dst8;
	int index;

	LLOGLN(0, ("convert_pixels: g_rdpScreen.depth: %d g_rdpScreen.rdp_bpp: %d num_pixels: %d",
			g_rdpScreen.depth, g_rdpScreen.rdp_bpp, num_pixels));

	if (g_rdpScreen.depth == g_rdpScreen.rdp_bpp)
	{
		memcpy(dst, src, num_pixels * g_Bpp);
		return 0;
	}

	if (g_rdpScreen.depth == 24)
	{
		src32 = (unsigned int *)src;

		if (g_rdpScreen.rdp_bpp == 32)
		{
			dst32 = (unsigned int*) dst;

			for (index = 0; index < num_pixels; index++)
			{
				pixel = *src32;
				*dst32 = pixel;
				dst32++;
				src32++;
			}
		}
		else if (g_rdpScreen.rdp_bpp == 24)
		{
			dst32 = (unsigned int *)dst;

			for (index = 0; index < num_pixels; index++)
			{
				pixel = *src32;
				*dst32 = pixel;
				dst32++;
				src32++;
			}
		}
		else if (g_rdpScreen.rdp_bpp == 16)
		{
			dst16 = (unsigned short *)dst;

			for (index = 0; index < num_pixels; index++)
			{
				pixel = *src32;
				SPLITCOLOR32(red, green, blue, pixel);
				pixel = COLOR16(red, green, blue);
				*dst16 = pixel;
				dst16++;
				src32++;
			}
		}
		else if (g_rdpScreen.rdp_bpp == 15)
		{
			dst16 = (unsigned short *)dst;

			for (index = 0; index < num_pixels; index++)
			{
				pixel = *src32;
				SPLITCOLOR32(red, green, blue, pixel);
				pixel = COLOR15(red, green, blue);
				*dst16 = pixel;
				dst16++;
				src32++;
			}
		}
		else if (g_rdpScreen.rdp_bpp == 8)
		{
			dst8 = (unsigned char *)dst;

			for (index = 0; index < num_pixels; index++)
			{
				pixel = *src32;
				SPLITCOLOR32(red, green, blue, pixel);
				pixel = COLOR8(red, green, blue);
				*dst8 = pixel;
				dst8++;
				src32++;
			}
		}
	}

	return 0;
}

static int get_single_color(struct image_data *id, int x, int y, int w, int h)
{
	int rv;
	int i;
	int j;
	int p;
	unsigned char *i8;
	unsigned short *i16;
	unsigned int *i32;

	p = 0;
	rv = -1;

	if (g_Bpp == 1)
	{
		for (i = 0; i < h; i++)
		{
			i8 = (unsigned char *)(id->pixels +
					((y + i) * id->lineBytes) + (x * g_Bpp));

			if (i == 0)
			{
				p = *i8;
			}

			for (j = 0; j < w; j++)
			{
				if (i8[j] != p)
				{
					return -1;
				}
			}
		}

		rv = p;
	}
	else if (g_Bpp == 2)
	{
		for (i = 0; i < h; i++)
		{
			i16 = (unsigned short *)(id->pixels +
					((y + i) * id->lineBytes) + (x * g_Bpp));

			if (i == 0)
			{
				p = *i16;
			}

			for (j = 0; j < w; j++)
			{
				if (i16[j] != p)
				{
					return -1;
				}
			}
		}

		rv = p;
	}
	else if (g_Bpp == 4)
	{
		for (i = 0; i < h; i++)
		{
			i32 = (unsigned int *)(id->pixels +
					((y + i) * id->lineBytes) + (x * g_Bpp));

			if (i == 0)
			{
				p = *i32;
			}

			for (j = 0; j < w; j++)
			{
				if (i32[j] != p)
				{
					return -1;
				}
			}
		}

		rv = p;
	}

	return rv;
}

void rdpup_send_area_rfx(struct image_data* id, int x, int y, int w, int h);

static int rdpup_disconnect(void)
{
	int index;

	RemoveEnabledDevice(g_sck);
	g_connected = 0;
	g_tcp_close(g_sck);
	g_sck = 0;
	g_sck_closed = 1;
	g_pixmap_byte_total = 0;
	g_pixmap_num_used = 0;
	g_rdpindex = -1;

	if (g_max_os_bitmaps > 0)
	{
		for (index = 0; index < g_max_os_bitmaps; index++)
		{
			if (g_os_bitmaps[index].used)
			{
				if (g_os_bitmaps[index].priv != 0)
				{
					g_os_bitmaps[index].priv->status = 0;
				}
			}
		}
	}

	g_max_os_bitmaps = 0;
	g_free(g_os_bitmaps);
	g_os_bitmaps = 0;
	g_use_rail = 0;
	return 0;
}

int rdpup_add_os_bitmap(PixmapPtr pixmap, rdpPixmapPtr priv)
{
	int index;
	int rv;
	int oldest;
	int oldest_index;

	if (!g_connected)
	{
		return -1;
	}

	if (g_os_bitmaps == 0)
	{
		return -1;
	}

	rv = -1;
	index = 0;

	while (index < g_max_os_bitmaps)
	{
		if (g_os_bitmaps[index].used == 0)
		{
			g_os_bitmaps[index].used = 1;
			g_os_bitmaps[index].pixmap = pixmap;
			g_os_bitmaps[index].priv = priv;
			g_os_bitmaps[index].stamp = g_os_bitmap_stamp;
			g_os_bitmap_stamp++;
			g_pixmap_num_used++;
			rv = index;
			break;
		}

		index++;
	}

	if (rv == -1)
	{
		/* find oldest */
		oldest = 0x7fffffff;
		oldest_index = 0;
		index = 0;

		while (index < g_max_os_bitmaps)
		{
			if (g_os_bitmaps[index].stamp < oldest)
			{
				oldest = g_os_bitmaps[index].stamp;
				oldest_index = index;
			}

			index++;
		}

		LLOGLN(10, ("rdpup_add_os_bitmap: evicting old, oldest_index %d", oldest_index));
		/* evict old */
		g_os_bitmaps[oldest_index].priv->status = 0;
		g_os_bitmaps[oldest_index].priv->con_number = 0;
		/* set new */
		g_os_bitmaps[oldest_index].pixmap = pixmap;
		g_os_bitmaps[oldest_index].priv = priv;
		g_os_bitmaps[oldest_index].stamp = g_os_bitmap_stamp;
		g_os_bitmap_stamp++;
		rv = oldest_index;
	}

	LLOGLN(10, ("rdpup_add_os_bitmap: new bitmap index %d", rv));
	LLOGLN(10, ("  g_pixmap_num_used %d", g_pixmap_num_used));
	return rv;
}

int rdpup_remove_os_bitmap(int rdpindex)
{
	LLOGLN(10, ("rdpup_remove_os_bitmap: index %d stamp %d",
			rdpindex, g_os_bitmaps[rdpindex].stamp));

	if (g_os_bitmaps == 0)
	{
		return 1;
	}

	if ((rdpindex < 0) && (rdpindex >= g_max_os_bitmaps))
	{
		return 1;
	}

	if (g_os_bitmaps[rdpindex].used)
	{
		g_os_bitmaps[rdpindex].used = 0;
		g_os_bitmaps[rdpindex].pixmap = 0;
		g_os_bitmaps[rdpindex].priv = 0;
		g_pixmap_num_used--;
	}

	LLOGLN(10, ("  g_pixmap_num_used %d", g_pixmap_num_used));
	return 0;
}

/* returns error */
static int rdpup_send(BYTE* data, int len)
{
	int sent;

	LLOGLN(10, ("rdpup_send - sending %d bytes", len));

	if (g_sck_closed)
	{
		return 1;
	}

	while (len > 0)
	{
		sent = g_tcp_send(g_sck, data, len, 0);

		if (sent == -1)
		{
			if (g_tcp_last_error_would_block(g_sck))
			{
				g_sleep(1);
			}
			else
			{
				rdpup_disconnect();
				return 1;
			}
		}
		else if (sent == 0)
		{
			rdpup_disconnect();
			return 1;
		}
		else
		{
			data += sent;
			len -= sent;
		}
	}

	return 0;
}

static int rdpup_send_msg(wStream* s)
{
	int length;
	int status;

	status = 1;

	if (s != 0)
	{
		length = Stream_GetPosition(s);

		if (length > Stream_Capacity(s))
		{
			rdpLog("overrun error len %d count %d\n", length, g_count);
		}

		Stream_SetPosition(s, 0);
		Stream_Write_UINT16(s, 3);
		Stream_Write_UINT16(s, g_count);
		Stream_Write_UINT32(s, length - 8);

		status = rdpup_send(s->buffer, length);
	}

	if (status != 0)
	{
		rdpLog("error in rdpup_send_msg\n");
	}

	return status;
}

int rdpup_update(XRDP_MSG_COMMON* msg);

static int rdpup_send_pending(void)
{
	XRDP_MSG_END_UPDATE msg;

	if (g_connected && g_begin)
	{
		LLOGLN(10, ("end %d", g_count));

		msg.type = XRDP_SERVER_END_UPDATE;
		rdpup_update((XRDP_MSG_COMMON*) &msg);
	}

	return 0;
}

static CARD32 rdpDeferredUpdateCallback(OsTimerPtr timer, CARD32 now, pointer arg)
{
	LLOGLN(10, ("rdpDeferredUpdateCallback"));

	if (g_do_dirty_ons)
	{
		rdpup_check_dirty_screen(&g_screenPriv);
	}
	else
	{
		rdpup_send_pending();
	}

	g_scheduled = 0;
	return 0;
}

void rdpScheduleDeferredUpdate(void)
{
	if (!g_scheduled)
	{
		g_scheduled = 1;
		g_timer = TimerSet(g_timer, 0, 40, rdpDeferredUpdateCallback, 0);
	}
}

/* returns error */
static int rdpup_recv(BYTE* data, int len)
{
	int rcvd;

	if (g_sck_closed)
	{
		return 1;
	}

	while (len > 0)
	{
		rcvd = g_tcp_recv(g_sck, data, len, 0);

		if (rcvd == -1)
		{
			if (g_tcp_last_error_would_block(g_sck))
			{
				g_sleep(1);
			}
			else
			{
				rdpup_disconnect();
				return 1;
			}
		}
		else if (rcvd == 0)
		{
			rdpup_disconnect();
			return 1;
		}
		else
		{
			data += rcvd;
			len -= rcvd;
		}
	}

	return 0;
}

static int rdpup_recv_msg(wStream* s)
{
	int len;
	int rv;

	rv = 1;

	if (s != 0)
	{
		Stream_EnsureCapacity(s, 4);
		Stream_SetPosition(s, 0);

		rv = rdpup_recv(s->buffer, 4);

		if (rv == 0)
		{
			Stream_Read_UINT32(s, len);

			if (len > 3)
			{
				Stream_EnsureCapacity(s, len);
				Stream_SetPosition(s, 0);

				rv = rdpup_recv(s->buffer, len - 4);
			}
		}
	}

	if (rv != 0)
	{
		rdpLog("error in rdpup_recv_msg\n");
	}

	return rv;
}

/*
    this from miScreenInit
    pScreen->mmWidth = (xsize * 254 + dpix * 5) / (dpix * 10);
    pScreen->mmHeight = (ysize * 254 + dpiy * 5) / (dpiy * 10);
 */
static int process_screen_size_msg(int width, int height, int bpp)
{
	RRScreenSizePtr pSize;
	int mmwidth;
	int mmheight;
	Bool ok;

	LLOGLN(0, ("process_screen_size_msg: set width %d height %d bpp %d",
			width, height, bpp));
	g_rdpScreen.rdp_width = width;
	g_rdpScreen.rdp_height = height;
	g_rdpScreen.rdp_bpp = bpp;

	if (bpp < 15)
	{
		g_rdpScreen.rdp_Bpp = 1;
		g_rdpScreen.rdp_Bpp_mask = 0xFF;
	}
	else if (bpp == 15)
	{
		g_rdpScreen.rdp_Bpp = 2;
		g_rdpScreen.rdp_Bpp_mask = 0x7FFF;
	}
	else if (bpp == 16)
	{
		g_rdpScreen.rdp_Bpp = 2;
		g_rdpScreen.rdp_Bpp_mask = 0xFFFF;
	}
	else if (bpp > 16)
	{
		g_rdpScreen.rdp_Bpp = 4;
		g_rdpScreen.rdp_Bpp_mask = 0xFFFFFF;
	}

	mmwidth = PixelToMM(width);
	mmheight = PixelToMM(height);

	pSize = RRRegisterSize(g_pScreen, width, height, mmwidth, mmheight);
	RRSetCurrentConfig(g_pScreen, RR_Rotate_0, 0, pSize);

	if ((g_rdpScreen.width != width) || (g_rdpScreen.height != height))
	{
		LLOGLN(0, ("  calling RRScreenSizeSet"));
		ok = RRScreenSizeSet(g_pScreen, width, height, mmwidth, mmheight);
		LLOGLN(0, ("  RRScreenSizeSet ok=[%d]", ok));
	}

	return 0;
}

static int l_bound_by(int val, int low, int high)
{
	if (val > high)
	{
		val = high;
	}

	if (val < low)
	{
		val = low;
	}

	return val;
}

static int rdpup_send_caps(void)
{
	int len;
	int rv;
	int cap_count;
	int cap_bytes;
	wStream* s;

	s = Stream_New(NULL, 8192);
	Stream_Seek(s, 8);

	cap_count = 0;
	cap_bytes = 0;

	len = (int) Stream_GetPosition(s);
	Stream_SetPosition(s, 0);

	Stream_Write_UINT16(s, 2); /* caps */
	Stream_Write_UINT16(s, cap_count); /* num caps */
	Stream_Write_UINT32(s, cap_bytes); /* caps len after header */

	rv = rdpup_send(s->buffer, len);

	if (rv != 0)
	{
		LLOGLN(0, ("rdpup_send_caps: rdpup_send failed"));
	}

	Stream_Free(s, TRUE);

	return rv;
}

static int process_version_msg(int param1, int param2, int param3, int param4)
{
	LLOGLN(0, ("process_version_msg: version %d %d %d %d", param1, param2,
			param3, param4));

	if ((param1 > 0) || (param2 > 0) || (param3 > 0) || (param4 > 0))
	{
		rdpup_send_caps();
	}

	return 0;
}

static int rdpup_send_rail(void)
{
	WindowPtr wnd;
	rdpWindowRec *priv;

	wnd = g_pScreen->root;

	if (wnd != 0)
	{
		wnd = wnd->lastChild;

		while (wnd != 0)
		{
			if (wnd->realized)
			{
				priv = GETWINPRIV(wnd);
				priv->status = 1;
				rdpup_create_window(wnd, priv);
			}

			wnd = wnd->prevSib;
		}
	}

	return 0;
}

const char CAPABILITIES_SCHEMA[] =
"{\"type\":\"record\",\
	\"name\":\"Capabilities\",\
	\"fields\":[\
		{\"name\": \"JPEG\", \"type\": \"boolean\"},\
		{\"name\": \"NSCodec\", \"type\": \"boolean\"},\
		{\"name\": \"RemoteFX\", \"type\": \"boolean\"},\
		{\"name\": \"OffscreenSupportLevel\", \"type\": \"int\"},\
		{\"name\": \"OffscreenCacheSize\", \"type\": \"int\"},\
		{\"name\": \"OffscreenCacheEntries\", \"type\": \"int\"},\
		{\"name\": \"RailSupportLevel\", \"type\": \"int\"},\
		{\"name\": \"PointerFlags\", \"type\": \"int\"}\
		]}";

static int rdpup_process_capabilities_msg(BYTE* buffer, int length)
{
	size_t index;

	avro_schema_t record_schema;
	avro_schema_from_json_literal(CAPABILITIES_SCHEMA, &record_schema);

	avro_value_iface_t* record_class = avro_generic_class_from_schema(record_schema);

	avro_reader_t reader = avro_reader_memory((char*) buffer, length);

	avro_value_t val;
	avro_generic_value_new(record_class, &val);

	avro_value_read(reader, &val);

	avro_value_t field;

	avro_value_get_by_name(&val, "JPEG", &field, &index);
	avro_value_get_boolean(&field, &g_rdpScreen.Jpeg);

	avro_value_get_by_name(&val, "NSCodec", &field, &index);
	avro_value_get_boolean(&field, &g_rdpScreen.NSCodec);

	avro_value_get_by_name(&val, "RemoteFX", &field, &index);
	avro_value_get_boolean(&field, &g_rdpScreen.RemoteFX);

	avro_value_get_by_name(&val, "OffscreenSupportLevel", &field, &index);
	avro_value_get_int(&field, &g_rdpScreen.OffscreenSupportLevel);

	avro_value_get_by_name(&val, "OffscreenCacheSize", &field, &index);
	avro_value_get_int(&field, &g_rdpScreen.OffscreenCacheSize);

	avro_value_get_by_name(&val, "OffscreenCacheEntries", &field, &index);
	avro_value_get_int(&field, &g_rdpScreen.OffscreenCacheEntries);

	avro_value_get_by_name(&val, "RailSupportLevel", &field, &index);
	avro_value_get_int(&field, &g_rdpScreen.RailSupportLevel);

	avro_value_get_by_name(&val, "PointerFlags", &field, &index);
	avro_value_get_int(&field, &g_rdpScreen.PointerFlags);

	LLOGLN(0, ("rdpup_process_capabilities_msg: JPEG %d NSCodec: %d RemoteFX: %d",
			g_rdpScreen.Jpeg, g_rdpScreen.NSCodec, g_rdpScreen.RemoteFX));

	if (g_rdpScreen.RemoteFX || g_rdpScreen.NSCodec)
	{
		g_rdpScreen.CodecMode = 1;
	}

	if (!g_rdpScreen.CodecMode)
	{
		if (g_rdpScreen.OffscreenSupportLevel > 0)
		{
			if (g_rdpScreen.OffscreenCacheEntries > 0)
			{
				g_max_os_bitmaps = g_rdpScreen.OffscreenCacheEntries;
				g_free(g_os_bitmaps);
				g_os_bitmaps = (struct rdpup_os_bitmap*)
					       g_malloc(sizeof(struct rdpup_os_bitmap) * g_max_os_bitmaps, 1);
			}
		}
	}
	else
	{
		g_os_bitmaps = NULL;
		g_max_os_bitmaps = 0;
	}

	if (g_rdpScreen.RailSupportLevel > 0)
	{
		g_use_rail = 1;
		rdpup_send_rail();
	}

	g_can_do_pix_to_pix = 0;

	if (g_rdpScreen.OffscreenCacheEntries == 2000)
		g_can_do_pix_to_pix = 1;

	return 0;
}

static int rdpup_process_msg(wStream* s)
{
	int msg_type;
	int msg;
	int param1;
	int param2;
	int param3;
	int param4;
	int bytes;

	Stream_Read_UINT16(s, msg_type);

	LLOGLN(10, ("rdpup_process_msg - msg %d", msg_type));

	if (msg_type == 103)
	{
		Stream_Read_UINT32(s, msg);
		Stream_Read_UINT32(s, param1);
		Stream_Read_UINT32(s, param2);
		Stream_Read_UINT32(s, param3);
		Stream_Read_UINT32(s, param4);

		LLOGLN(10, ("rdpup_process_msg - msg %d param1 %d param2 %d param3 %d "
				"param4 %d", msg, param1, param2, param3, param4));

		switch (msg)
		{
			case 15: /* key down */
			case 16: /* key up */
				KbdAddEvent(msg == 15, param1, param2, param3, param4);
				break;
			case 17: /* from RDP_INPUT_SYNCHRONIZE */
				KbdSync(param1);
				break;
			case 100:
				/* without the minus 2, strange things happen when dragging
                   	   	   past the width or height */
				g_cursor_x = l_bound_by(param1, 0, g_rdpScreen.width - 2);
				g_cursor_y = l_bound_by(param2, 0, g_rdpScreen.height - 2);
				PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
				break;
			case 101:
				g_button_mask = g_button_mask & (~1);
				PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
				break;
			case 102:
				g_button_mask = g_button_mask | 1;
				PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
				break;
			case 103:
				g_button_mask = g_button_mask & (~4);
				PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
				break;
			case 104:
				g_button_mask = g_button_mask | 4;
				PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
				break;
			case 105:
				g_button_mask = g_button_mask & (~2);
				PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
				break;
			case 106:
				g_button_mask = g_button_mask | 2;
				PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
				break;
			case 107:
				g_button_mask = g_button_mask & (~8);
				PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
				break;
			case 108:
				g_button_mask = g_button_mask | 8;
				PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
				break;
			case 109:
				g_button_mask = g_button_mask & (~16);
				PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
				break;
			case 110:
				g_button_mask = g_button_mask | 16;
				PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
				break;
			case 200:
				rdpup_begin_update();
				rdpup_send_area(0, (param1 >> 16) & 0xFFFF, param1 & 0xFFFF,
						(param2 >> 16) & 0xFFFF, param2 & 0xFFFF);
				rdpup_end_update();
				break;
			case 300:
				process_screen_size_msg(param1, param2, param3);
				break;
			case 301:
				process_version_msg(param1, param2, param3, param4);
				break;
		}
	}
	else if (msg_type == 104)
	{
		bytes = s->capacity - 6;
		rdpup_process_capabilities_msg(s->pointer, bytes);
	}
	else
	{
		rdpLog("unknown message type in rdpup_process_msg %d\n", msg_type);
	}

	return 0;
}

void rdpup_get_screen_image_rect(struct image_data *id)
{
	id->width = g_rdpScreen.width;
	id->height = g_rdpScreen.height;
	id->bpp = g_rdpScreen.rdp_bpp;
	id->Bpp = g_rdpScreen.rdp_Bpp;
	id->lineBytes = g_rdpScreen.paddedWidthInBytes;
	id->pixels = g_rdpScreen.pfbMemory;
}

void rdpup_get_pixmap_image_rect(PixmapPtr pPixmap, struct image_data *id)
{
	id->width = pPixmap->drawable.width;
	id->height = pPixmap->drawable.height;
	id->bpp = g_rdpScreen.rdp_bpp;
	id->Bpp = g_rdpScreen.rdp_Bpp;
	id->lineBytes = pPixmap->devKind;
	id->pixels = (char *)(pPixmap->devPrivate.ptr);
}

int rdpup_init(void)
{
	int i;
	char text[256];

	if (!g_directory_exist("/tmp/.xrdp"))
	{
		if (!g_create_dir("/tmp/.xrdp"))
		{
			LLOGLN(0, ("rdpup_init: g_create_dir failed"));
			return 0;
		}

		g_chmod_hex("/tmp/.xrdp", 0x1777);
	}

	i = atoi(display);

	if (i < 1)
	{
		return 0;
	}

	if (g_in_s == 0)
	{
		g_in_s = Stream_New(NULL, 8192);
	}

	if (g_out_s == 0)
	{
		g_out_s = Stream_New(NULL, 1920 * 1088 * g_Bpp + 100);
	}

	pfbBackBufferMemory = (BYTE*) malloc(g_rdpScreen.sizeInBytes);

	if (g_use_uds)
	{
		g_sprintf(g_uds_data, "/tmp/.xrdp/xrdp_display_%s", display);

		if (g_listen_sck == 0)
		{
			g_listen_sck = g_tcp_local_socket_stream();

			if (g_tcp_local_bind(g_listen_sck, g_uds_data) != 0)
			{
				LLOGLN(0, ("rdpup_init: g_tcp_local_bind failed"));
				return 0;
			}

			g_tcp_listen(g_listen_sck);
			AddEnabledDevice(g_listen_sck);
		}
	}
	else
	{
		g_sprintf(text, "62%2.2d", i);

		if (g_listen_sck == 0)
		{
			g_listen_sck = g_tcp_socket();

			if (g_tcp_bind(g_listen_sck, text) != 0)
			{
				return 0;
			}

			g_tcp_listen(g_listen_sck);
			AddEnabledDevice(g_listen_sck);
		}
	}

	g_dis_listen_sck = g_tcp_local_socket_dgram();

	if (g_dis_listen_sck != 0)
	{
		g_sprintf(text, "/tmp/.xrdp/xrdp_disconnect_display_%s", display);

		if (g_tcp_local_bind(g_dis_listen_sck, text) == 0)
		{
			AddEnabledDevice(g_dis_listen_sck);
		}
		else
		{
			rdpLog("g_tcp_local_bind failed [%s]\n", text);
		}
	}

	return 1;
}

int rdpup_check(void)
{
	int sel;
	int new_sck;
	char buf[8];

	sel = g_tcp_select3(g_listen_sck, g_sck, g_dis_listen_sck);

	if (sel & 1)
	{
		new_sck = g_tcp_accept(g_listen_sck);

		if (new_sck == -1)
		{
		}
		else
		{
			if (g_sck != 0)
			{
				/* should maybe ask is user wants to allow here with timeout */
				rdpLog("replacing connection, already got a connection\n");
				rdpup_disconnect();
			}

			rdpLog("got a connection\n");
			g_sck = new_sck;
			g_tcp_set_non_blocking(g_sck);
			g_tcp_set_no_delay(g_sck);
			g_connected = 1;
			g_sck_closed = 0;
			g_begin = 0;
			g_con_number++;
			AddEnabledDevice(g_sck);
		}
	}

	if (sel & 2)
	{
		if (rdpup_recv_msg(g_in_s) == 0)
		{
			rdpup_process_msg(g_in_s);
		}
	}

	if (sel & 4)
	{
		if (g_tcp_recv(g_dis_listen_sck, buf, 4, 0) > 0)
		{
			if (g_sck != 0)
			{
				rdpLog("disconnecting session via user request\n");
				rdpup_disconnect();
			}
		}
	}

	return 0;
}

int rdpup_begin_update(void)
{
	XRDP_MSG_BEGIN_UPDATE msg;

	if (g_connected)
	{
		if (g_begin)
		{
			return 0;
		}

		Stream_SetPosition(g_out_s, 0);
		Stream_Seek(g_out_s, 8);

		LLOGLN(10, ("begin %d", g_count));

		msg.type = XRDP_SERVER_BEGIN_UPDATE;
		rdpup_update((XRDP_MSG_COMMON*) &msg);
	}

	return 0;
}

int rdpup_end_update(void)
{
	XRDP_MSG_END_UPDATE msg;

	LLOGLN(10, ("rdpup_end_update"));

	if (g_connected && g_begin)
	{
		if (g_do_dirty_ons)
		{
			rdpup_send_pending();
		}
		else
		{
#if 0
			rdpScheduleDeferredUpdate();
#else
			msg.type = XRDP_SERVER_END_UPDATE;
			rdpup_update((XRDP_MSG_COMMON*) &msg);
#endif
		}
	}

	return 0;
}

int rdpup_update(XRDP_MSG_COMMON* msg)
{
	wStream* s = g_out_s;

	if (g_connected)
	{
		xrdp_prepare_msg(NULL, msg);

		if (!g_begin && (msg->type != XRDP_SERVER_BEGIN_UPDATE))
		{
			rdpup_begin_update();
		}

		if (msg->type == XRDP_SERVER_BEGIN_UPDATE)
		{
			if (g_begin)
				return 0;

			Stream_SetPosition(s, 0);
			Stream_Seek(s, 8);

			xrdp_prepare_msg(s, msg);

			g_begin = 1;
			g_count = 1;

			return 0;
		}
		else if (msg->type == XRDP_SERVER_END_UPDATE)
		{
			xrdp_prepare_msg(s, msg);
			g_count++;

			rdpup_send_msg(s);

			g_begin = 0;
			g_count = 0;

			return 0;
		}

		if ((Stream_GetPosition(s) + msg->length + 20) > Stream_Capacity(s))
		{
			rdpup_send_msg(s);

			g_begin = 0;
			g_count = 0;
		}

		xrdp_prepare_msg(s, msg);
		g_count++;
	}

	return 0;
}

int rdpup_fill_rect(short x, short y, int cx, int cy)
{
	XRDP_MSG_OPAQUE_RECT msg;

	if (g_rdpScreen.CodecMode)
	{
		rdpup_send_area(NULL, x, y, cx, cy);
		return 0;
	}

	msg.nLeftRect = x;
	msg.nTopRect = y;
	msg.nWidth = cx;
	msg.nHeight = cy;

	msg.type = XRDP_SERVER_OPAQUE_RECT;
	rdpup_update((XRDP_MSG_COMMON*) &msg);

	return 0;
}

int rdpup_screen_blt(short x, short y, int cx, int cy, short srcx, short srcy)
{
	XRDP_MSG_SCREEN_BLT msg;

	if (g_rdpScreen.CodecMode)
	{
		rdpup_send_area(NULL, x, y, cx, cy);
		return 0;
	}

	msg.nLeftRect = x;
	msg.nTopRect = y;
	msg.nWidth = cx;
	msg.nHeight = cy;
	msg.nXSrc = srcx;
	msg.nYSrc = srcy;

	msg.type = XRDP_SERVER_SCREEN_BLT;
	rdpup_update((XRDP_MSG_COMMON*) &msg);

	return 0;
}

int rdpup_set_clip(short x, short y, int cx, int cy)
{
	XRDP_MSG_SET_CLIP msg;

	msg.x = x;
	msg.y = y;
	msg.width = cx;
	msg.height = cy;

	msg.type = XRDP_SERVER_SET_CLIP;
	rdpup_update((XRDP_MSG_COMMON*) &msg);

	return 0;
}

int rdpup_reset_clip(void)
{
	XRDP_MSG_RESET_CLIP msg;

	msg.type = XRDP_SERVER_RESET_CLIP;
	rdpup_update((XRDP_MSG_COMMON*) &msg);

	return 0;
}

int rdpup_set_fgcolor(int fgcolor)
{
	XRDP_MSG_SET_FORECOLOR msg;

	fgcolor = fgcolor & g_Bpp_mask;
	fgcolor = convert_pixel(fgcolor) & g_rdpScreen.rdp_Bpp_mask;
	msg.ForeColor = fgcolor;

	msg.type = XRDP_SERVER_SET_FORECOLOR;
	rdpup_update((XRDP_MSG_COMMON*) &msg);

	return 0;
}

int rdpup_set_bgcolor(int bgcolor)
{
	XRDP_MSG_SET_BACKCOLOR msg;

	bgcolor = bgcolor & g_Bpp_mask;
	bgcolor = convert_pixel(bgcolor) & g_rdpScreen.rdp_Bpp_mask;

	msg.BackColor = bgcolor;

	msg.type = XRDP_SERVER_SET_BACKCOLOR;
	rdpup_update((XRDP_MSG_COMMON*) &msg);

	return 0;
}

int rdpup_set_opcode(int opcode)
{
	XRDP_MSG_SET_ROP2 msg;

	msg.bRop2 = g_rdp_opcodes[opcode & 0xF];

	msg.type = XRDP_SERVER_SET_ROP2;
	rdpup_update((XRDP_MSG_COMMON*) &msg);

	return 0;
}

int rdpup_set_pen(int style, int width)
{
	XRDP_MSG_SET_PEN msg;

	msg.PenStyle = style;
	msg.PenWidth = width;

	msg.type = XRDP_SERVER_SET_PEN;
	rdpup_update((XRDP_MSG_COMMON*) &msg);

	return 0;
}

int rdpup_draw_line(short x1, short y1, short x2, short y2)
{
	XRDP_MSG_LINE_TO msg;

	msg.nXStart = x1;
	msg.nYStart = y1;
	msg.nXEnd = x2;
	msg.nYEnd = y2;

	msg.type = XRDP_SERVER_LINE_TO;
	rdpup_update((XRDP_MSG_COMMON*) &msg);

	return 0;
}

int rdpup_set_cursor(short x, short y, char *cur_data, char *cur_mask)
{
	XRDP_MSG_SET_POINTER msg;

	msg.xPos = x;
	msg.yPos = y;
	msg.xorMaskData = (BYTE*) cur_data;
	msg.andMaskData = (BYTE*) cur_mask;

	msg.type = XRDP_SERVER_SET_POINTER;
	rdpup_update((XRDP_MSG_COMMON*) &msg);

	return 0;
}

int rdpup_set_cursor_ex(short x, short y, char *cur_data, char *cur_mask, int bpp)
{
	XRDP_MSG_SET_POINTER_EX msg;

	msg.xPos = x;
	msg.yPos = y;
	msg.xorBpp = bpp;
	msg.xorMaskData = (BYTE*) cur_data;
	msg.andMaskData = (BYTE*) cur_mask;

	msg.type = XRDP_SERVER_SET_POINTER_EX;
	rdpup_update((XRDP_MSG_COMMON*) &msg);

	return 0;
}

int rdpup_create_os_surface(int rdpindex, int width, int height)
{
	XRDP_MSG_CREATE_OS_SURFACE msg;

	msg.index = rdpindex;
	msg.width = width;
	msg.height = height;

	msg.type = XRDP_SERVER_CREATE_OS_SURFACE;
	rdpup_update((XRDP_MSG_COMMON*) &msg);

	return 0;
}

int rdpup_switch_os_surface(int rdpindex)
{
	XRDP_MSG_SWITCH_OS_SURFACE msg;

	if (g_rdpindex == rdpindex)
		return 0;

	g_rdpindex = rdpindex;

	msg.index = rdpindex;

	msg.type = XRDP_SERVER_SWITCH_OS_SURFACE;
	rdpup_update((XRDP_MSG_COMMON*) &msg);

	return 0;
}

int rdpup_delete_os_surface(int rdpindex)
{
	XRDP_MSG_DELETE_OS_SURFACE msg;

	msg.index = rdpindex;

	msg.type = XRDP_SERVER_DELETE_OS_SURFACE;
	rdpup_update((XRDP_MSG_COMMON*) &msg);

	return 0;
}

void rdpup_send_area_rfx(struct image_data* id, int x, int y, int w, int h)
{
	int i;
	char* s;
	int size;
	char* dstp;
	int bitmapLength;

	bitmapLength = w * h * g_Bpp;
	size = bitmapLength + 26;
	XRDP_MSG_PAINT_RECT msg;

	for (i = 0; i < h; i++)
	{
		dstp = (char*) &pfbBackBufferMemory[w * g_Bpp * i];
		s = (g_rdpScreen.pfbMemory + ((y + i) * g_rdpScreen.paddedWidthInBytes) + (x * g_Bpp));
		convert_pixels(s, dstp, w);
	}

	msg.nLeftRect = x;
	msg.nTopRect = y;
	msg.nWidth = w;
	msg.nHeight = h;
	msg.nXSrc = 0;
	msg.nYSrc = 0;
	msg.bitmapData = (BYTE*) pfbBackBufferMemory;
	msg.bitmapDataLength = bitmapLength;

	msg.type = XRDP_SERVER_PAINT_RECT;
	rdpup_update((XRDP_MSG_COMMON*) &msg);
}

/* split the bitmap up into 64 x 64 pixel areas */
void rdpup_send_area(struct image_data *id, int x, int y, int w, int h)
{
	char *s;
	int i;
	int lx;
	int ly;
	int lh;
	int lw;
	int size;
	char* dstp;
	int single_color;
	int bitmapLength;
	struct image_data lid;

	if (id == 0)
	{
		rdpup_get_screen_image_rect(&lid);
		id = &lid;
	}

	if (g_rdpScreen.CodecMode)
	{
		rdpup_send_area_rfx(id, x, y, w, h);
		return;
	}

	if (x >= id->width)
	{
		return;
	}

	if (y >= id->height)
	{
		return;
	}

	if (x < 0)
	{
		w += x;
		x = 0;
	}

	if (y < 0)
	{
		h += y;
		y = 0;
	}

	if (w <= 0)
	{
		return;
	}

	if (h <= 0)
	{
		return;
	}

	if (x + w > id->width)
	{
		w = id->width - x;
	}

	if (y + h > id->height)
	{
		h = id->height - y;
	}

	ly = y;

	while (ly < y + h)
	{
		lx = x;

		while (lx < x + w)
		{
			lw = MIN(64, (x + w) - lx);
			lh = MIN(64, (y + h) - ly);
			single_color = get_single_color(id, lx, ly, lw, lh);

			if (single_color != -1)
			{
				LLOGLN(10, ("%d sending single color", g_count));
				rdpup_set_fgcolor(single_color);
				rdpup_fill_rect(lx, ly, lw, lh);
			}
			else
			{
				bitmapLength = lw * lh * id->Bpp;
				size = bitmapLength + 26;
				XRDP_MSG_PAINT_RECT msg;

				for (i = 0; i < lh; i++)
				{
					dstp = (char*) &pfbBackBufferMemory[lw * id->Bpp * i];
					s = (id->pixels + ((ly + i) * id->lineBytes) + (lx * g_Bpp));
					convert_pixels(s, dstp, lw);
				}

				msg.nLeftRect = lx;
				msg.nTopRect = ly;
				msg.nWidth = lw;
				msg.nHeight = lh;
				msg.nXSrc = 0;
				msg.nYSrc = 0;
				msg.bitmapData = (BYTE*) pfbBackBufferMemory;
				msg.bitmapDataLength = bitmapLength;

				msg.type = XRDP_SERVER_PAINT_RECT;
				rdpup_update((XRDP_MSG_COMMON*) &msg);
			}

			lx += 64;
		}

		ly += 64;
	}
}

void rdpup_paint_rect_os(int x, int y, int cx, int cy, int rdpindex, int srcx, int srcy)
{
	XRDP_MSG_MEMBLT msg;

	msg.nLeftRect = x;
	msg.nTopRect = y;
	msg.nWidth = cx;
	msg.nHeight = cy;
	msg.index = rdpindex;
	msg.nXSrc = srcx;
	msg.nYSrc = srcy;

	msg.type = XRDP_SERVER_MEMBLT;
	rdpup_update((XRDP_MSG_COMMON*) &msg);
}

void rdpup_set_hints(int hints, int mask)
{
	XRDP_MSG_SET_HINTS msg;

	msg.hints = hints;
	msg.mask = mask;

	msg.type = XRDP_SERVER_SET_HINTS;
	rdpup_update((XRDP_MSG_COMMON*) &msg);
}

void rdpup_create_framebuffer(XRDP_MSG_CREATE_FRAMEBUFFER* msg)
{
	msg->type = XRDP_SERVER_CREATE_FRAMEBUFFER;
	rdpup_update((XRDP_MSG_COMMON*) msg);
}

void rdpup_create_window(WindowPtr pWindow, rdpWindowRec *priv)
{
	RECTANGLE_16 windowRects;
	RECTANGLE_16 visibilityRects;
	XRDP_MSG_WINDOW_NEW_UPDATE msg;

	msg.rootParentHandle = (UINT32) pWindow->drawable.pScreen->root->drawable.id;

	if (pWindow->overrideRedirect)
	{
		msg.style = XR_STYLE_TOOLTIP;
		msg.extendedStyle = XR_EXT_STYLE_TOOLTIP;
	}
	else
	{
		msg.style = XR_STYLE_NORMAL;
		msg.extendedStyle = XR_EXT_STYLE_NORMAL;
	}

	msg.titleInfo.string = (BYTE*) _strdup("title");
	msg.titleInfo.length = strlen((char*) msg.titleInfo.string);

	msg.windowId = (UINT32) pWindow->drawable.id;
	msg.ownerWindowId = (UINT32) pWindow->parent->drawable.id;

	msg.showState = 0;

	msg.clientOffsetX = 0;
	msg.clientOffsetY = 0;

	msg.clientAreaWidth = pWindow->drawable.width;
	msg.clientAreaHeight = pWindow->drawable.height;

	msg.RPContent = 0;

	msg.windowOffsetX = pWindow->drawable.x;
	msg.windowOffsetY = pWindow->drawable.y;

	msg.windowClientDeltaX = 0;
	msg.windowClientDeltaY = 0;

	msg.windowWidth = pWindow->drawable.width;
	msg.windowHeight = pWindow->drawable.height;

	msg.numWindowRects = 1;
	msg.windowRects = (RECTANGLE_16*) &windowRects;
	msg.windowRects[0].left = 0;
	msg.windowRects[0].top = 0;
	msg.windowRects[0].right = pWindow->drawable.width;
	msg.windowRects[0].bottom = pWindow->drawable.height;

	msg.numVisibilityRects = 1;
	msg.visibilityRects = (RECTANGLE_16*) &visibilityRects;
	msg.visibilityRects[0].left = 0;
	msg.visibilityRects[0].top = 0;
	msg.visibilityRects[0].right = pWindow->drawable.width;
	msg.visibilityRects[0].bottom = pWindow->drawable.height;

	msg.visibleOffsetX = pWindow->drawable.x;
	msg.visibleOffsetY = pWindow->drawable.y;

	msg.type = XRDP_SERVER_WINDOW_NEW_UPDATE;
	rdpup_update((XRDP_MSG_COMMON*) &msg);

	free(msg.titleInfo.string);
}

void rdpup_delete_window(WindowPtr pWindow, rdpWindowRec *priv)
{
	XRDP_MSG_WINDOW_DELETE msg;

	msg.windowId = (UINT32) pWindow->drawable.id;

	msg.type = XRDP_SERVER_WINDOW_DELETE;
	rdpup_update((XRDP_MSG_COMMON*) &msg);
}

int rdpup_check_dirty(PixmapPtr pDirtyPixmap, rdpPixmapRec *pDirtyPriv)
{
	int index;
	int clip_index;
	int count;
	int num_clips;
	BoxRec box;
	xSegment *seg;
	struct image_data id;
	struct rdp_draw_item *di;

	if (pDirtyPriv == 0)
	{
		return 0;
	}

	if (pDirtyPriv->is_dirty == 0)
	{
		return 0;
	}

	/* update use time / count */
	g_os_bitmaps[pDirtyPriv->rdpindex].stamp = g_os_bitmap_stamp;
	g_os_bitmap_stamp++;

	LLOGLN(10, ("-----------------got dirty"));
	rdpup_switch_os_surface(pDirtyPriv->rdpindex);
	rdpup_get_pixmap_image_rect(pDirtyPixmap, &id);
	rdpup_begin_update();
	draw_item_pack(pDirtyPixmap, pDirtyPriv);
	di = pDirtyPriv->draw_item_head;

	while (di != 0)
	{
		LLOGLN(10, ("rdpup_check_dirty: type %d", di->type));

		switch (di->type)
		{
			case RDI_FILL:
				rdpup_set_fgcolor(di->u.fill.fg_color);
				rdpup_set_opcode(di->u.fill.opcode);
				count = REGION_NUM_RECTS(di->reg);

				for (index = 0; index < count; index++)
				{
					box = REGION_RECTS(di->reg)[index];
					LLOGLN(10, ("  RDI_FILL %d %d %d %d", box.x1, box.y1,
							box.x2, box.y2));
					rdpup_fill_rect(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
				}

				rdpup_set_opcode(GXcopy);
				break;

			case RDI_IMGLL:
				rdpup_set_hints(1, 1);
				rdpup_set_opcode(di->u.img.opcode);
				count = REGION_NUM_RECTS(di->reg);

				for (index = 0; index < count; index++)
				{
					box = REGION_RECTS(di->reg)[index];
					LLOGLN(10, ("  RDI_IMGLL %d %d %d %d", box.x1, box.y1,
							box.x2, box.y2));
					rdpup_send_area(&id, box.x1, box.y1, box.x2 - box.x1,
							box.y2 - box.y1);
				}

				rdpup_set_opcode(GXcopy);
				rdpup_set_hints(0, 1);
				break;

			case RDI_IMGLY:
				rdpup_set_opcode(di->u.img.opcode);
				count = REGION_NUM_RECTS(di->reg);

				for (index = 0; index < count; index++)
				{
					box = REGION_RECTS(di->reg)[index];
					LLOGLN(10, ("  RDI_IMGLY %d %d %d %d", box.x1, box.y1,
							box.x2, box.y2));
					rdpup_send_area(&id, box.x1, box.y1, box.x2 - box.x1,
							box.y2 - box.y1);
				}

				rdpup_set_opcode(GXcopy);
				break;

			case RDI_LINE:
				LLOGLN(10, ("  RDI_LINE"));
				num_clips = REGION_NUM_RECTS(di->reg);

				if (num_clips > 0)
				{
					rdpup_set_fgcolor(di->u.line.fg_color);
					rdpup_set_opcode(di->u.line.opcode);
					rdpup_set_pen(0, di->u.line.width);

					for (clip_index = num_clips - 1; clip_index >= 0; clip_index--)
					{
						box = REGION_RECTS(di->reg)[clip_index];
						rdpup_set_clip(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);

						for (index = 0; index < di->u.line.nseg; index++)
						{
							seg = di->u.line.segs + index;
							LLOGLN(10, ("  RDI_LINE %d %d %d %d", seg->x1, seg->y1,
									seg->x2, seg->y2));
							rdpup_draw_line(seg->x1, seg->y1, seg->x2, seg->y2);
						}
					}
				}

				rdpup_reset_clip();
				rdpup_set_opcode(GXcopy);
				break;

			case RDI_SCRBLT:
				LLOGLN(10, ("  RDI_SCRBLT"));
				break;
		}

		di = di->next;
	}

	draw_item_remove_all(pDirtyPriv);
	rdpup_end_update();
	pDirtyPriv->is_dirty = 0;
	rdpup_switch_os_surface(-1);
	return 0;
}

int rdpup_check_dirty_screen(rdpPixmapRec *pDirtyPriv)
{
	int index;
	int clip_index;
	int count;
	int num_clips;
	BoxRec box;
	xSegment *seg;
	struct image_data id;
	struct rdp_draw_item *di;

	if (pDirtyPriv == 0)
	{
		return 0;
	}

	if (pDirtyPriv->is_dirty == 0)
	{
		return 0;
	}

	LLOGLN(10, ("-----------------got dirty"));
	rdpup_get_screen_image_rect(&id);
	rdpup_begin_update();
	draw_item_pack(0, pDirtyPriv);
	di = pDirtyPriv->draw_item_head;

	while (di != 0)
	{
		LLOGLN(10, ("rdpup_check_dirty_screen: type %d", di->type));

		switch (di->type)
		{
			case RDI_FILL:
				rdpup_set_fgcolor(di->u.fill.fg_color);
				rdpup_set_opcode(di->u.fill.opcode);
				count = REGION_NUM_RECTS(di->reg);

				for (index = 0; index < count; index++)
				{
					box = REGION_RECTS(di->reg)[index];
					LLOGLN(10, ("  RDI_FILL %d %d %d %d", box.x1, box.y1,
							box.x2, box.y2));
					rdpup_fill_rect(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
				}

				rdpup_set_opcode(GXcopy);
				break;

			case RDI_IMGLL:
				rdpup_set_hints(1, 1);
				rdpup_set_opcode(di->u.img.opcode);
				count = REGION_NUM_RECTS(di->reg);

				for (index = 0; index < count; index++)
				{
					box = REGION_RECTS(di->reg)[index];
					LLOGLN(10, ("  RDI_IMGLL %d %d %d %d", box.x1, box.y1,
							box.x2, box.y2));
					rdpup_send_area(&id, box.x1, box.y1, box.x2 - box.x1,
							box.y2 - box.y1);
				}

				rdpup_set_opcode(GXcopy);
				rdpup_set_hints(0, 1);
				break;

			case RDI_IMGLY:
				rdpup_set_opcode(di->u.img.opcode);
				count = REGION_NUM_RECTS(di->reg);

				for (index = 0; index < count; index++)
				{
					box = REGION_RECTS(di->reg)[index];
					LLOGLN(10, ("  RDI_IMGLY %d %d %d %d", box.x1, box.y1,
							box.x2, box.y2));
					rdpup_send_area(&id, box.x1, box.y1, box.x2 - box.x1,
							box.y2 - box.y1);
				}

				rdpup_set_opcode(GXcopy);
				break;

			case RDI_LINE:
				LLOGLN(10, ("  RDI_LINE"));
				num_clips = REGION_NUM_RECTS(di->reg);

				if (num_clips > 0)
				{
					rdpup_set_fgcolor(di->u.line.fg_color);
					rdpup_set_opcode(di->u.line.opcode);
					rdpup_set_pen(0, di->u.line.width);

					for (clip_index = num_clips - 1; clip_index >= 0; clip_index--)
					{
						box = REGION_RECTS(di->reg)[clip_index];
						rdpup_set_clip(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);

						for (index = 0; index < di->u.line.nseg; index++)
						{
							seg = di->u.line.segs + index;
							LLOGLN(10, ("  RDI_LINE %d %d %d %d", seg->x1, seg->y1,
									seg->x2, seg->y2));
							rdpup_draw_line(seg->x1, seg->y1, seg->x2, seg->y2);
						}
					}
				}

				rdpup_reset_clip();
				rdpup_set_opcode(GXcopy);
				break;

			case RDI_SCRBLT:
				LLOGLN(10, ("  RDI_SCRBLT"));
				break;
		}

		di = di->next;
	}

	draw_item_remove_all(pDirtyPriv);
	rdpup_end_update();
	pDirtyPriv->is_dirty = 0;
	return 0;
}
