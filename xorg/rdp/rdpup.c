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
static int g_count = 0;

static BYTE* pfbBackBufferMemory = NULL;

extern DevPrivateKeyRec g_rdpWindowIndex;
extern ScreenPtr g_pScreen;
extern int g_Bpp;
extern int g_Bpp_mask;
extern rdpScreenInfoRec g_rdpScreen;
extern int g_use_rail;
extern int g_use_uds;
extern char g_uds_data[];
extern int g_con_number;

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

/**
 * DstBlt:
 *
 * GXclear	BLACKNESS 	0x00000042
 * GXnoop	D		0x00AA0029
 * GXinvert	DSTINVERT	0x00550009
 * GXset	WHITENESS	0x00FF0062
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

	LLOGLN(10, ("convert_pixels: g_rdpScreen.depth: %d g_rdpScreen.rdp_bpp: %d num_pixels: %d",
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

static int rdpup_disconnect(void)
{
	RemoveEnabledDevice(g_sck);
	g_connected = 0;
	g_tcp_close(g_sck);
	g_sck = 0;
	g_sck_closed = 1;
	g_pixmap_byte_total = 0;
	g_pixmap_num_used = 0;
	g_use_rail = 0;

	return 0;
}

/* returns error */
static int rdpup_send(BYTE* data, int len)
{
	int sent;

	LLOGLN(10, ("rdpup_send - sending %d bytes", len));

	if (g_sck_closed)
		return 1;

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
		Stream_Write_UINT32(s, length);
		Stream_Write_UINT32(s, g_count);

		status = rdpup_send(Stream_Buffer(s), length);
	}

	if (status != 0)
	{
		rdpLog("error in rdpup_send_msg\n");
	}

	return status;
}

int rdpup_update(XRDP_MSG_COMMON* msg);

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

static int rdpup_recv_msg(wStream* s, int* type)
{
	int status = 1;
	XRDP_MSG_COMMON common;

	if (s != 0)
	{
		Stream_EnsureCapacity(s, 10);
		Stream_SetPosition(s, 0);

		status = rdpup_recv(Stream_Pointer(s), 10);

		if (status == 0)
		{
			xrdp_read_common_header(s, &common);

			if (common.length >= 10)
			{
				Stream_EnsureCapacity(s, common.length);
				status = rdpup_recv(Stream_Pointer(s), common.length - 10);
			}
		}
	}

	if (status != 0)
	{
		rdpLog("error in rdpup_recv_msg\n");
	}

	Stream_SetPosition(s, common.length);
	Stream_SealLength(s);
	Stream_SetPosition(s, 10);

	*type = common.type;

	return status;
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

static int process_screen_parameters(int DesktopWidth, int DesktopHeight, int ColorDepth)
{
	Bool ok;
	int mmwidth;
	int mmheight;
	RRScreenSizePtr pSize;

	g_rdpScreen.rdp_width = DesktopWidth;
	g_rdpScreen.rdp_height = DesktopHeight;
	g_rdpScreen.rdp_bpp = ColorDepth;

	if (ColorDepth < 15)
	{
		g_rdpScreen.rdp_Bpp = 1;
		g_rdpScreen.rdp_Bpp_mask = 0xFF;
	}
	else if (ColorDepth == 15)
	{
		g_rdpScreen.rdp_Bpp = 2;
		g_rdpScreen.rdp_Bpp_mask = 0x7FFF;
	}
	else if (ColorDepth == 16)
	{
		g_rdpScreen.rdp_Bpp = 2;
		g_rdpScreen.rdp_Bpp_mask = 0xFFFF;
	}
	else if (ColorDepth > 16)
	{
		g_rdpScreen.rdp_Bpp = 4;
		g_rdpScreen.rdp_Bpp_mask = 0xFFFFFF;
	}

	mmwidth = PixelToMM(DesktopWidth);
	mmheight = PixelToMM(DesktopHeight);

	pSize = RRRegisterSize(g_pScreen, DesktopWidth, DesktopHeight, mmwidth, mmheight);
	RRSetCurrentConfig(g_pScreen, RR_Rotate_0, 0, pSize);

	if ((g_rdpScreen.width != DesktopWidth) || (g_rdpScreen.height != DesktopHeight))
	{
		LLOGLN(0, ("  calling RRScreenSizeSet"));
		ok = RRScreenSizeSet(g_pScreen, DesktopWidth, DesktopHeight, mmwidth, mmheight);
		LLOGLN(0, ("  RRScreenSizeSet ok=[%d]", ok));
	}

	return 0;
}

static int rdpup_process_capabilities_msg(wStream* s)
{
	XRDP_MSG_CAPABILITIES msg;

	xrdp_read_capabilities(s, &msg);

	g_rdpScreen.Jpeg = (msg.SupportedCodecs & XRDP_CODEC_JPEG) ? TRUE : FALSE;
	g_rdpScreen.NSCodec = (msg.SupportedCodecs & XRDP_CODEC_NSCODEC) ? TRUE : FALSE;
	g_rdpScreen.RemoteFX = (msg.SupportedCodecs & XRDP_CODEC_REMOTEFX) ? TRUE : FALSE;
	g_rdpScreen.CodecMode = msg.SupportedCodecs ? TRUE : FALSE;

	g_rdpScreen.RailSupportLevel = msg.RailSupportLevel;
	g_rdpScreen.PointerFlags = msg.PointerFlags;

	LLOGLN(0, ("rdpup_process_capabilities_msg: JPEG %d NSCodec: %d RemoteFX: %d",
			g_rdpScreen.Jpeg, g_rdpScreen.NSCodec, g_rdpScreen.RemoteFX));

	if (g_rdpScreen.RailSupportLevel > 0)
	{
		g_use_rail = 1;
		rdpup_send_rail();
	}

	process_screen_parameters(msg.DesktopWidth, msg.DesktopHeight, msg.ColorDepth);

	return 0;
}

int rdpup_process_refresh_rect_msg(wStream* s, XRDP_MSG_REFRESH_RECT* msg)
{
	int index;

	Stream_Read_UINT16(s, msg->numberOfAreas);

	msg->areasToRefresh = (RECTANGLE_16*) Stream_Pointer(s);

	for (index = 0; index < msg->numberOfAreas; index++)
	{
		Stream_Read_UINT16(s, msg->areasToRefresh[index].left);
		Stream_Read_UINT16(s, msg->areasToRefresh[index].top);
		Stream_Read_UINT16(s, msg->areasToRefresh[index].right);
		Stream_Read_UINT16(s, msg->areasToRefresh[index].bottom);
	}

	return 0;
}

static int rdpup_process_msg(wStream* s, int type)
{
	LLOGLN(10, ("rdpup_process_msg - msg %d", type));

	if (type == XRDP_CLIENT_EVENT)
	{
		XRDP_MSG_EVENT msg;

		xrdp_read_event(s, &msg);

		LLOGLN(10, ("rdpup_process_msg - subtype %d param1 %d param2 %d param3 %d "
				"param4 %d", msg.subType, msg.param1, msg.param2, msg.param3, msg.param4));

		switch (msg.subType)
		{
			case 15: /* key down */
			case 16: /* key up */
				KbdAddEvent(msg.subType == 15, msg.param1, msg.param2, msg.param3, msg.param4);
				break;

			case 17: /* from RDP_INPUT_SYNCHRONIZE */
				KbdSync(msg.param1);
				break;

			case 100:
				/* without the minus 2, strange things happen when dragging
                   	   	   past the width or height */
				g_cursor_x = l_bound_by(msg.param1, 0, g_rdpScreen.width - 2);
				g_cursor_y = l_bound_by(msg.param2, 0, g_rdpScreen.height - 2);
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
		}
	}
	else if (type == XRDP_CLIENT_CAPABILITIES)
	{
		rdpup_process_capabilities_msg(s);
	}
	else if (type == XRDP_CLIENT_REFRESH_RECT)
	{
		int index;
		XRDP_MSG_REFRESH_RECT msg;

		xrdp_read_refresh_rect(s, &msg);

		rdpup_begin_update();

		for (index = 0; index < msg.numberOfAreas; index++)
		{
			rdpup_send_area(NULL, msg.areasToRefresh[index].left, msg.areasToRefresh[index].top,
					msg.areasToRefresh[index].right - msg.areasToRefresh[index].left + 1,
					msg.areasToRefresh[index].bottom - msg.areasToRefresh[index].top + 1);
		}

		rdpup_end_update();
	}
	else
	{
		rdpLog("unknown message type in rdpup_process_msg %d\n", type);
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

UINT32 rdpup_convert_color(UINT32 color)
{
	color = color & g_Bpp_mask;
	color = convert_pixel(color) & g_rdpScreen.rdp_Bpp_mask;
	return color;
}

UINT32 rdpup_convert_opcode(int opcode)
{
	return g_rdp_opcodes[opcode & 0xF];
}

UINT32 rdp_dstblt_rop(int opcode)
{
	UINT32 rop = 0;

	switch (opcode)
	{
		case GXclear:
			rop = GDI_BLACKNESS;
			break;

		case GXset:
			rop = GDI_WHITENESS;
			break;

		case GXnoop:
			rop = GDI_D;
			break;

		case GXinvert:
			rop = GDI_DSTINVERT;
			break;

		default:
			rop = 0;
			break;
	}

	return rop;
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
			g_rdpScreen.fbAttached = 0;
			AddEnabledDevice(g_sck);
		}
	}

	if (sel & 2)
	{
		int type = 0;

		if (rdpup_recv_msg(g_in_s, &type) == 0)
		{
			rdpup_process_msg(g_in_s, type);
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
		msg.type = XRDP_SERVER_END_UPDATE;
		rdpup_update((XRDP_MSG_COMMON*) &msg);
	}

	return 0;
}

int rdpup_update(XRDP_MSG_COMMON* msg)
{
	wStream* s = g_out_s;

	if (g_connected)
	{
		xrdp_server_message_write(NULL, msg);

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

			xrdp_server_message_write(s, msg);

			g_begin = 1;
			g_count = 1;

			return 0;
		}
		else if (msg->type == XRDP_SERVER_END_UPDATE)
		{
			xrdp_server_message_write(s, msg);
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

		xrdp_server_message_write(s, msg);
		g_count++;

		LLOGLN(0, ("rdpup_update: adding %s message (%d)", xrdp_server_message_name(msg->type), msg->type));
	}
	else
	{
		//LLOGLN(0, ("rdpup_update: discarding %s message (%d)", xrdp_server_message_name(msg->type), msg->type));
	}

	return 0;
}

int rdpup_opaque_rect(XRDP_MSG_OPAQUE_RECT* msg)
{
	if (g_rdpScreen.CodecMode)
	{
		rdpup_send_area(NULL, msg->nLeftRect, msg->nTopRect, msg->nWidth, msg->nHeight);
		return 0;
	}

	msg->type = XRDP_SERVER_OPAQUE_RECT;
	rdpup_update((XRDP_MSG_COMMON*) msg);

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

int rdpup_patblt(XRDP_MSG_PATBLT* msg)
{
	if (g_rdpScreen.CodecMode)
	{
		rdpup_send_area(NULL, msg->nLeftRect, msg->nTopRect, msg->nWidth, msg->nHeight);
		return 0;
	}

	msg->type = XRDP_SERVER_PATBLT;
	rdpup_update((XRDP_MSG_COMMON*) msg);

	return 0;
}

int rdpup_dstblt(XRDP_MSG_DSTBLT* msg)
{
	if (g_rdpScreen.CodecMode)
	{
		rdpup_send_area(NULL, msg->nLeftRect, msg->nTopRect, msg->nWidth, msg->nHeight);
		return 0;
	}

	msg->type = XRDP_SERVER_DSTBLT;
	rdpup_update((XRDP_MSG_COMMON*) msg);

	return 0;
}

int rdpup_set_clipping_region(XRDP_MSG_SET_CLIPPING_REGION* msg)
{
	msg->type = XRDP_SERVER_SET_CLIPPING_REGION;
	rdpup_update((XRDP_MSG_COMMON*) msg);

	return 0;
}

int rdpup_set_clip(short x, short y, int cx, int cy)
{
	XRDP_MSG_SET_CLIPPING_REGION msg;

	msg.bNullRegion = 0;
	msg.nLeftRect = x;
	msg.nTopRect = y;
	msg.nWidth = cx;
	msg.nHeight = cy;

	rdpup_set_clipping_region(&msg);

	return 0;
}

int rdpup_reset_clip(void)
{
	XRDP_MSG_SET_CLIPPING_REGION msg;

	msg.bNullRegion = 1;
	msg.nLeftRect = 0;
	msg.nTopRect = 0;
	msg.nWidth = 0;
	msg.nHeight = 0;

	rdpup_set_clipping_region(&msg);

	return 0;
}

int rdpup_draw_line(XRDP_MSG_LINE_TO* msg)
{
	msg->type = XRDP_SERVER_LINE_TO;
	rdpup_update((XRDP_MSG_COMMON*) msg);

	return 0;
}

int rdpup_set_pointer(XRDP_MSG_SET_POINTER* msg)
{
	msg->type = XRDP_SERVER_SET_POINTER;
	rdpup_update((XRDP_MSG_COMMON*) msg);

	return 0;
}

void rdpup_send_area_codec(struct image_data* id, int x, int y, int w, int h)
{
	int i;
	char* s;
	char* dstp;
	int bitmapLength;
	XRDP_MSG_PAINT_RECT msg;

	if (x < 0)
		x = 0;

	if (x > g_rdpScreen.width - 1)
		x = g_rdpScreen.width - 1;

	w += x % 16;
	x -= x % 16;

	w += w % 16;

	if (x + w > g_rdpScreen.width)
		w = g_rdpScreen.width - x;

	if (y < 0)
		y = 0;

	if (y > g_rdpScreen.height - 1)
		y = g_rdpScreen.height - 1;

	h += y % 16;
	y -= y % 16;

	h += h % 16;

	if (h > g_rdpScreen.height)
		h = g_rdpScreen.height;

	if (y + h > g_rdpScreen.height)
		h = g_rdpScreen.height - y;

	if (w * h < 1)
		return;

	bitmapLength = w * h * g_Bpp;

	if (g_rdpScreen.sharedMemory && !g_rdpScreen.fbAttached)
	{
		XRDP_MSG_SHARED_FRAMEBUFFER msg;

		msg.attach = 1;
		msg.width = g_rdpScreen.width;
		msg.height = g_rdpScreen.height;
		msg.scanline = g_rdpScreen.paddedWidthInBytes;
		msg.segmentId = g_rdpScreen.segmentId;
		msg.bitsPerPixel = g_rdpScreen.depth;
		msg.bytesPerPixel = g_Bpp;

		msg.type = XRDP_SERVER_SHARED_FRAMEBUFFER;
		rdpup_update((XRDP_MSG_COMMON*) &msg);

		g_rdpScreen.fbAttached = 1;
	}

	if (!g_rdpScreen.sharedMemory)
	{
		for (i = 0; i < h; i++)
		{
			dstp = (char*) &pfbBackBufferMemory[w * g_Bpp * i];
			s = (g_rdpScreen.pfbMemory + ((y + i) * g_rdpScreen.paddedWidthInBytes) + (x * g_Bpp));
			convert_pixels(s, dstp, w);
		}
	}

	msg.nLeftRect = x;
	msg.nTopRect = y;
	msg.nWidth = w;
	msg.nHeight = h;
	msg.nXSrc = 0;
	msg.nYSrc = 0;

	if (g_rdpScreen.sharedMemory)
	{
		msg.fbSegmentId = g_rdpScreen.segmentId;
		msg.bitmapData = NULL;
		msg.bitmapDataLength = 0;
	}
	else
	{
		msg.fbSegmentId = 0;
		msg.bitmapData = (BYTE*) pfbBackBufferMemory;
		msg.bitmapDataLength = bitmapLength;
	}

	msg.type = XRDP_SERVER_PAINT_RECT;
	rdpup_update((XRDP_MSG_COMMON*) &msg);
}

/* split the bitmap up into 64 x 64 pixel areas */
void rdpup_send_area(struct image_data *id, int x, int y, int w, int h)
{
	char* s;
	int i;
	int lx;
	int ly;
	int lh;
	int lw;
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
		rdpup_send_area_codec(id, x, y, w, h);
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
				XRDP_MSG_OPAQUE_RECT msg;

				LLOGLN(10, ("%d sending single color", g_count));

				msg.nLeftRect = lx;
				msg.nTopRect = ly;
				msg.nWidth = lw;
				msg.nHeight = lh;
				msg.color = rdpup_convert_color(single_color);

				rdpup_opaque_rect(&msg);
			}
			else
			{
				XRDP_MSG_PAINT_RECT msg;

				bitmapLength = lw * lh * id->Bpp;

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

				msg.fbSegmentId = 0;
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

void rdpup_shared_framebuffer(XRDP_MSG_SHARED_FRAMEBUFFER* msg)
{
	msg->type = XRDP_SERVER_SHARED_FRAMEBUFFER;
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

