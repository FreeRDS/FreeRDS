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

#include <winpr/crt.h>
#include <winpr/pipe.h>
#include <winpr/synch.h>

#include <freerds/backend.h>

#define LOG_LEVEL 0
#define LLOG(_level, _args) \
		do { if (_level < LOG_LEVEL) { ErrorF _args ; } } while (0)
#define LLOGLN(_level, _args) \
		do { if (_level < LOG_LEVEL) { ErrorF _args ; ErrorF("\n"); } } while (0)

static int g_clientfd = -1;
static rdsBackendService* g_service;
static int g_connected = 0;

static int g_button_mask = 0;
static BYTE* pfbBackBufferMemory = NULL;

extern ScreenPtr g_pScreen;
extern int g_Bpp;
extern int g_Bpp_mask;
extern rdpScreenInfoRec g_rdpScreen;
extern int g_con_number;

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

int rdpup_update(RDS_MSG_COMMON* msg);

int rdpup_process_refresh_rect_msg(wStream* s, RDS_MSG_REFRESH_RECT* msg)
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

int rdpup_begin_update(void)
{
	return 0;
}

int rdpup_end_update(void)
{
	return 0;
}

int rdpup_update(RDS_MSG_COMMON* msg)
{
	int status;

	if (g_connected)
	{
		if ((msg->type == RDS_SERVER_BEGIN_UPDATE) ||
				(msg->type == RDS_SERVER_END_UPDATE))
		{
			return 0;
		}

		status = freerds_server_outbound_write_message((rdsBackend *)g_service, (RDS_MSG_COMMON*) msg);

		LLOGLN(0, ("rdpup_update: adding %s message (%d)", freerds_server_message_name(msg->type), msg->type));
	}
	else
	{
		LLOGLN(0, ("rdpup_update: discarding %s message (%d)", freerds_server_message_name(msg->type), msg->type));
	}

	return 0;
}

int rdpup_check_attach_framebuffer()
{
	if (g_rdpScreen.sharedMemory && !g_rdpScreen.fbAttached)
	{
		RDS_MSG_SHARED_FRAMEBUFFER msg;

		msg.flags = RDS_FRAMEBUFFER_FLAG_ATTACH;
		msg.width = g_rdpScreen.width;
		msg.height = g_rdpScreen.height;
		msg.scanline = g_rdpScreen.paddedWidthInBytes;
		msg.segmentId = g_rdpScreen.segmentId;
		msg.bitsPerPixel = g_rdpScreen.depth;
		msg.bytesPerPixel = g_Bpp;

		msg.type = RDS_SERVER_SHARED_FRAMEBUFFER;
		rdpup_update((RDS_MSG_COMMON*) &msg);

		g_rdpScreen.fbAttached = 1;
	}

	return 0;
}

int rdpup_detach_framebuffer()
{
	if (g_rdpScreen.sharedMemory && g_rdpScreen.fbAttached)
	{
		RDS_MSG_SHARED_FRAMEBUFFER msg;

		msg.flags = 0;
		msg.width = g_rdpScreen.width;
		msg.height = g_rdpScreen.height;
		msg.scanline = g_rdpScreen.paddedWidthInBytes;
		msg.segmentId = g_rdpScreen.segmentId;
		msg.bitsPerPixel = g_rdpScreen.depth;
		msg.bytesPerPixel = g_Bpp;

		msg.type = RDS_SERVER_SHARED_FRAMEBUFFER;
		rdpup_update((RDS_MSG_COMMON*) &msg);

		g_rdpScreen.fbAttached = 0;
	}

	return 0;
}

int rdpup_opaque_rect(RDS_MSG_OPAQUE_RECT* msg)
{
	rdpup_check_attach_framebuffer();

	msg->type = RDS_SERVER_OPAQUE_RECT;
	rdpup_update((RDS_MSG_COMMON*) msg);

	return 0;
}

int rdpup_screen_blt(short x, short y, int cx, int cy, short srcx, short srcy)
{
	RDS_MSG_SCREEN_BLT msg;

	rdpup_check_attach_framebuffer();

	msg.nLeftRect = x;
	msg.nTopRect = y;
	msg.nWidth = cx;
	msg.nHeight = cy;
	msg.nXSrc = srcx;
	msg.nYSrc = srcy;

	msg.type = RDS_SERVER_SCREEN_BLT;
	rdpup_update((RDS_MSG_COMMON*) &msg);

	return 0;
}

int rdpup_patblt(RDS_MSG_PATBLT* msg)
{
	rdpup_check_attach_framebuffer();

	msg->type = RDS_SERVER_PATBLT;
	rdpup_update((RDS_MSG_COMMON*) msg);

	return 0;
}

int rdpup_dstblt(RDS_MSG_DSTBLT* msg)
{
	rdpup_check_attach_framebuffer();

	msg->type = RDS_SERVER_DSTBLT;
	rdpup_update((RDS_MSG_COMMON*) msg);

	return 0;
}

int rdpup_set_clipping_region(RDS_MSG_SET_CLIPPING_REGION* msg)
{
	msg->type = RDS_SERVER_SET_CLIPPING_REGION;
	rdpup_update((RDS_MSG_COMMON*) msg);

	return 0;
}

int rdpup_set_clip(short x, short y, int cx, int cy)
{
	RDS_MSG_SET_CLIPPING_REGION msg;

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
	RDS_MSG_SET_CLIPPING_REGION msg;

	msg.bNullRegion = 1;
	msg.nLeftRect = 0;
	msg.nTopRect = 0;
	msg.nWidth = 0;
	msg.nHeight = 0;

	rdpup_set_clipping_region(&msg);

	return 0;
}

int rdpup_draw_line(RDS_MSG_LINE_TO* msg)
{
	msg->type = RDS_SERVER_LINE_TO;
	rdpup_update((RDS_MSG_COMMON*) msg);

	return 0;
}

int rdpup_set_pointer(RDS_MSG_SET_POINTER* msg)
{
	msg->type = RDS_SERVER_SET_POINTER;
	rdpup_update((RDS_MSG_COMMON*) msg);

	return 0;
}

void rdpup_send_area(int x, int y, int w, int h)
{
	int bitmapLength;
	RDS_MSG_PAINT_RECT msg;

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

	rdpup_check_attach_framebuffer();

	msg.nLeftRect = x;
	msg.nTopRect = y;
	msg.nWidth = w;
	msg.nHeight = h;
	msg.nXSrc = 0;
	msg.nYSrc = 0;

	msg.fbSegmentId = g_rdpScreen.segmentId;
	msg.bitmapData = NULL;
	msg.bitmapDataLength = 0;

	msg.type = RDS_SERVER_PAINT_RECT;
	rdpup_update((RDS_MSG_COMMON*) &msg);
}

void rdpup_shared_framebuffer(RDS_MSG_SHARED_FRAMEBUFFER* msg)
{
	msg->type = RDS_SERVER_SHARED_FRAMEBUFFER;
	rdpup_update((RDS_MSG_COMMON*) msg);
}

void rdpup_create_window(WindowPtr pWindow, rdpWindowRec *priv)
{
	RECTANGLE_16 windowRects;
	RECTANGLE_16 visibilityRects;
	RDS_MSG_WINDOW_NEW_UPDATE msg;

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

	msg.type = RDS_SERVER_WINDOW_NEW_UPDATE;
	rdpup_update((RDS_MSG_COMMON*) &msg);

	free(msg.titleInfo.string);
}

void rdpup_delete_window(WindowPtr pWindow, rdpWindowRec *priv)
{
	RDS_MSG_WINDOW_DELETE msg;

	msg.windowId = (UINT32) pWindow->drawable.id;

	msg.type = RDS_SERVER_WINDOW_DELETE;
	rdpup_update((RDS_MSG_COMMON*) &msg);
}

int rds_client_capabilities(rdsBackend* backend, RDS_MSG_CAPABILITIES* capabilities)
{
	int width;
	int height;
	int mmWidth;
	int mmHeight;

	width = capabilities->DesktopWidth;
	height = capabilities->DesktopHeight;

	mmWidth = PixelToMM(width);
	mmHeight = PixelToMM(height);

	RRScreenSizeSet(g_pScreen, width, height, mmWidth, mmHeight);

	RRTellChanged(g_pScreen);

	return 0;
}

int rds_client_synchronize_keyboard_event(rdsBackend* backend, DWORD flags)
{
	return 0;
}

int rds_client_scancode_keyboard_event(rdsBackend* backend, DWORD flags, DWORD code, DWORD keyboardType)
{
	KbdAddScancodeEvent(flags, code, keyboardType);
	return 0;
}

int rds_client_virtual_keyboard_event(rdsBackend* backend, DWORD flags, DWORD code)
{
	KbdAddVirtualKeyCodeEvent(flags, code);
	return 0;
}

int rds_client_unicode_keyboard_event(rdsBackend* backend, DWORD flags, DWORD code)
{
	KbdAddUnicodeEvent(flags, code);
	return 0;
}

int rds_client_mouse_event(rdsBackend* backend, DWORD flags, DWORD x, DWORD y)
{
	if (x > g_rdpScreen.width - 2)
		x = g_rdpScreen.width - 2;

	if (y > g_rdpScreen.height - 2)
		y = g_rdpScreen.height - 2;

	if ((flags & PTR_FLAGS_MOVE) || (flags & PTR_FLAGS_DOWN))
	{
		PtrAddMotionEvent(x, y);
	}

	if (flags & PTR_FLAGS_WHEEL)
	{
		if (flags & PTR_FLAGS_WHEEL_NEGATIVE)
		{
			g_button_mask = g_button_mask | 16;
			PtrAddButtonEvent(g_button_mask);

			g_button_mask = g_button_mask & (~16);
			PtrAddButtonEvent(g_button_mask);
		}
		else
		{
			g_button_mask = g_button_mask | 8;
			PtrAddButtonEvent(g_button_mask);

			g_button_mask = g_button_mask & (~8);
			PtrAddButtonEvent(g_button_mask);
		}
	}
	else if (flags & PTR_FLAGS_BUTTON1)
	{
		if (flags & PTR_FLAGS_DOWN)
		{
			g_button_mask = g_button_mask | 1;
			PtrAddButtonEvent(g_button_mask);
		}
		else
		{
			g_button_mask = g_button_mask & (~1);
			PtrAddButtonEvent(g_button_mask);
		}
	}
	else if (flags & PTR_FLAGS_BUTTON2)
	{
		if (flags & PTR_FLAGS_DOWN)
		{
			g_button_mask = g_button_mask | 4;
			PtrAddButtonEvent(g_button_mask);
		}
		else
		{
			g_button_mask = g_button_mask & (~4);
			PtrAddButtonEvent(g_button_mask);
		}
	}
	else if (flags & PTR_FLAGS_BUTTON3)
	{
		if (flags & PTR_FLAGS_DOWN)
		{
			g_button_mask = g_button_mask | 2;
			PtrAddButtonEvent(g_button_mask);
		}
		else
		{
			g_button_mask = g_button_mask & (~2);
			PtrAddButtonEvent(g_button_mask);
		}
	}

	return 0;
}

int rds_client_extended_mouse_event(rdsBackend* backend, DWORD flags, DWORD x, DWORD y)
{
	if (x > g_rdpScreen.width - 2)
		x = g_rdpScreen.width - 2;

	if (y > g_rdpScreen.height - 2)
		y = g_rdpScreen.height - 2;

	if ((flags & PTR_FLAGS_MOVE) || (flags & PTR_XFLAGS_DOWN))
	{
		PtrAddMotionEvent(x, y);
	}

	if (flags & PTR_XFLAGS_BUTTON1)
	{
		if (flags & PTR_XFLAGS_DOWN)
		{
			g_button_mask = g_button_mask | (1<<7);
			PtrAddButtonEvent(g_button_mask);
		}
		else
		{
			g_button_mask = g_button_mask & (~(1<<7));
			PtrAddButtonEvent(g_button_mask);
		}
	}
	else if (flags & PTR_XFLAGS_BUTTON2)
	{
		if (flags & PTR_XFLAGS_DOWN)
		{
			g_button_mask = g_button_mask | (1<<8);
			PtrAddButtonEvent(g_button_mask);
		}
		else
		{
			g_button_mask = g_button_mask & (~(1<<8));
			PtrAddButtonEvent(g_button_mask);
		}
	}

	return 0;
}

int rds_service_accept(rdsBackendService* service)
{
	HANDLE hServerPipe;
	hServerPipe = service->hServerPipe;
	RemoveEnabledDevice(GetNamePipeFileDescriptor(hServerPipe));

	service->hServerPipe = freerds_named_pipe_create_endpoint(service->SessionId, service->Endpoint);

	if (!service->hServerPipe)
	{
		fprintf(stderr, "server pipe failed?!\n");
		return 1;
	}
	AddEnabledDevice(GetNamePipeFileDescriptor(service->hServerPipe));
	service->hClientPipe = freerds_named_pipe_accept(hServerPipe);

	g_clientfd = GetNamePipeFileDescriptor(service->hClientPipe);

	g_con_number++;
	g_connected = 1;
	g_rdpScreen.fbAttached = 0;

	AddEnabledDevice(g_clientfd);

	fprintf(stderr, "RdsServiceAccept\n");

	return 0;
}

int rds_service_disconnect(rdsBackendService* service)
{
	RemoveEnabledDevice(g_clientfd);

	CloseHandle(service->hClientPipe);
	service->hClientPipe = NULL;

	fprintf(stderr, "RdsServiceDisconnect\n");

	g_connected = 0;
	g_rdpScreen.fbAttached = 0;
	g_clientfd = 0;

	return 0;
}

int rdpup_init(void)
{
	int DisplayId;
	rdsBackendService* service;

	DisplayId = atoi(display);

	LLOGLN(0, ("rdpup_init: display: %d", DisplayId));

	if (DisplayId < 1)
	{
		return 0;
	}

	pfbBackBufferMemory = (BYTE*) malloc(g_rdpScreen.sizeInBytes);

	if (!g_service)
	{
		g_service = freerds_service_new(DisplayId, "X11");

		service = g_service;

		service->Accept = (pRdsServiceAccept) rds_service_accept;

		service->client->Capabilities = rds_client_capabilities;
		service->client->SynchronizeKeyboardEvent = rds_client_synchronize_keyboard_event;
		service->client->ScancodeKeyboardEvent = rds_client_scancode_keyboard_event;
		service->client->VirtualKeyboardEvent = rds_client_virtual_keyboard_event;
		service->client->UnicodeKeyboardEvent = rds_client_unicode_keyboard_event;
		service->client->MouseEvent = rds_client_mouse_event;
		service->client->ExtendedMouseEvent = rds_client_extended_mouse_event;
		service->hServerPipe = freerds_named_pipe_create_endpoint(service->SessionId, service->Endpoint);
		AddEnabledDevice(GetNamePipeFileDescriptor(service->hServerPipe));
	}

	return 1;
}

int rdpup_check(void)
{
	rdsBackendService* service = g_service;

	if (service->hClientPipe)
	{
		if (WaitForSingleObject(service->hClientPipe, 0) == WAIT_OBJECT_0)
		{
			if (freerds_transport_receive((rdsBackend *)service) < 0)
			{
				rds_service_disconnect(service);
			}
		}
	}
	else
	{
		if (WaitForSingleObject(service->hServerPipe, 0) == WAIT_OBJECT_0)
		{
			service->Accept(service);
		}
	}

	return 0;
}
