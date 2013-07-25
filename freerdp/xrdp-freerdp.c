/**
 * FreeRDP: A Remote Desktop Protocol Server
 * freerdp wrapper
 *
 * Copyright 2011-2012 Jay Sorg
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

#include "xrdp-freerdp.h"

#include "log.h"
#include "os_calls.h"
#include "defines.h"

#include <X11/Xlib.h>

#include "xrdp-color.h"

#include <winpr/crt.h>
#include <winpr/stream.h>

#include <freerdp/freerdp.h>
#include <freerdp/settings.h>
#include <freerdp/rail.h>
#include <freerdp/rail/rail.h>
#include <freerdp/codec/bitmap.h>

#ifdef XRDP_DEBUG
#define LOG_LEVEL 99
#else
#define LOG_LEVEL 0
#endif

#define LLOG(_level, _args) \
    do { if (_level < LOG_LEVEL) { g_write _args ; } } while (0)
#define LLOGLN(_level, _args) \
    do { if (_level < LOG_LEVEL) { g_writeln _args ; } } while (0)

struct mod_context
{
	rdpContext _p;
	xrdpModule* modi;
};
typedef struct mod_context modContext;

void verifyColorMap(xrdpModule* mod)
{
	int i;
	for (i = 0; i < 255; i++)
	{
		if (mod->colormap[i] != 0)
		{
			return;
		}
	}
	LLOGLN(0, ("The colormap is all NULL\n"));
}

static int freerdp_xrdp_client_start(xrdpModule* mod, int w, int h, int bpp)
{
	rdpSettings* settings;

	LLOGLN(10, ("freerdp_xrdp_client_start: w %d h %d bpp %d", w, h, bpp));

	settings = mod->instance->settings;
	settings->DesktopWidth = w;
	settings->DesktopHeight = h;
	settings->ColorDepth = bpp;
	mod->bpp = bpp;

	settings->TlsSecurity = TRUE;
	settings->NlaSecurity = FALSE;
	settings->RdpSecurity = TRUE;

	settings->AsyncTransport = TRUE;

	return 0;
}

static int freerdp_xrdp_client_connect(xrdpModule* mod)
{
	BOOL ok;

	LLOGLN(10, ("freerdp_xrdp_client_connect:"));

	ok = freerdp_connect(mod->instance);
	LLOGLN(0, ("freerdp_xrdp_client_connect: freerdp_connect returned %d", ok));

	if (!ok)
	{
		LLOGLN(0, ("Failure to connect"));
#ifdef ERRORSTART

		if (connectErrorCode != 0)
		{
			char buf[128];

			if (connectErrorCode < ERRORSTART)
			{
				if (strerror_r(connectErrorCode, buf, 128) != 0)
				{
					g_snprintf(buf, 128, "Errorcode from connect : %d", connectErrorCode);
				}
			}
			else
			{
				switch (connectErrorCode)
				{
					case PREECONNECTERROR:
						g_snprintf(buf, 128, "The error code from connect is "
							"PREECONNECTERROR");
						break;
					case UNDEFINEDCONNECTERROR:
						g_snprintf(buf, 128, "The error code from connect is "
							"UNDEFINEDCONNECTERROR");
						break;
					case POSTCONNECTERROR:
						g_snprintf(buf, 128, "The error code from connect is "
							"POSTCONNECTERROR");
						break;
					case DNSERROR:
						g_snprintf(buf, 128, "The DNS system generated an error");
						break;
					case DNSNAMENOTFOUND:
						g_snprintf(buf, 128, "The DNS system could not find the "
							"specified name");
						break;
					case CONNECTERROR:
						g_snprintf(buf, 128, "A general connect error was returned");
						break;
					case MCSCONNECTINITIALERROR:
						g_snprintf(buf, 128, "The error code from connect is "
							"MCSCONNECTINITIALERROR");
						break;
					case TLSCONNECTERROR:
						g_snprintf(buf, 128, "Error in TLS handshake");
						break;
					case AUTHENTICATIONERROR:
						g_snprintf(buf, 128, "Authentication error check your password "
							"and username");
						break;
					default:
						g_snprintf(buf, 128, "Unhandled Errorcode from connect : %d",
								connectErrorCode);
						break;
				}
			}
			log_message(LOG_LEVEL_INFO, buf);
			mod->server->Message(mod, buf, 0);
		}

#endif
		log_message(LOG_LEVEL_INFO, "freerdp_connect Failed to destination :%s:%d",
				mod->instance->settings->ServerHostname, mod->instance->settings->ServerPort);
		return 1;
	}
	else
	{
		log_message(LOG_LEVEL_INFO, "freerdp_connect returned Success to destination :%s:%d",
				mod->instance->settings->ServerHostname, mod->instance->settings->ServerPort);
	}

	return 0;
}

static int freerdp_xrdp_client_event(xrdpModule* mod, int msg, long param1, long param2, long param3, long param4)
{
	int x;
	int y;
	int flags;

	LLOGLN(12, ("freerdp_xrdp_client_event: msg %d", msg));

	switch (msg)
	{
		case 15: /* key down */
			mod->instance->input->KeyboardEvent(mod->instance->input, param4, param3);
			break;

		case 16: /* key up */
			mod->instance->input->KeyboardEvent(mod->instance->input, param4, param3);
			break;

		case 17: /*Synchronize*/
			LLOGLN(11, ("Synchronized event handled"));
			mod->instance->input->SynchronizeEvent(mod->instance->input, 0);
			break;

		case 100: /* mouse move */
			LLOGLN(12, ("mouse move %d %d", param1, param2));
			x = param1;
			y = param2;
			flags = PTR_FLAGS_MOVE;
			mod->instance->input->MouseEvent(mod->instance->input, flags, x, y);
			break;

		case 101: /* left button up */
			LLOGLN(12, ("left button up %d %d", param1, param2));
			x = param1;
			y = param2;
			flags = PTR_FLAGS_BUTTON1;
			mod->instance->input->MouseEvent(mod->instance->input, flags, x, y);
			break;

		case 102: /* left button down */
			LLOGLN(12, ("left button down %d %d", param1, param2));
			x = param1;
			y = param2;
			flags = PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN;
			mod->instance->input->MouseEvent(mod->instance->input, flags, x, y);
			break;

		case 103: /* right button up */
			LLOGLN(12, ("right button up %d %d", param1, param2));
			x = param1;
			y = param2;
			flags = PTR_FLAGS_BUTTON2;
			mod->instance->input->MouseEvent(mod->instance->input, flags, x, y);
			break;

		case 104: /* right button down */
			LLOGLN(12, ("right button down %d %d", param1, param2));
			x = param1;
			y = param2;
			flags = PTR_FLAGS_BUTTON2 | PTR_FLAGS_DOWN;
			mod->instance->input->MouseEvent(mod->instance->input, flags, x, y);
			break;

		case 105: /* middle button up */
			LLOGLN(12, ("middle button up %d %d", param1, param2));
			x = param1;
			y = param2;
			flags = PTR_FLAGS_BUTTON3;
			mod->instance->input->MouseEvent(mod->instance->input, flags, x, y);
			break;

		case 106: /* middle button down */
			LLOGLN(12, ("middle button down %d %d", param1, param2));
			x = param1;
			y = param2;
			flags = PTR_FLAGS_BUTTON3 | PTR_FLAGS_DOWN;
			mod->instance->input->MouseEvent(mod->instance->input, flags, x, y);
			break;

		case 107: /* wheel up */
			flags = PTR_FLAGS_WHEEL | 0x0078;
			mod->instance->input->MouseEvent(mod->instance->input, flags, 0, 0);
		case 108:
			break;

		case 109: /* wheel down */
			flags = PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | 0x0088;
			mod->instance->input->MouseEvent(mod->instance->input, flags, 0, 0);
		case 110:
			break;

		case 200:
			LLOGLN(12, ("Invalidate request sent from client"));
			RECTANGLE_16 *rectangle = (RECTANGLE_16 *) g_malloc(sizeof(RECTANGLE_16), 0);
			/* The parameters are coded as follows param1 = MAKELONG(y, x), param2 =MAKELONG(h, w)
			 * #define MAKELONG(lo, hi) ((((hi) & 0xffff) << 16) | ((lo) & 0xffff))
			 */
			rectangle->left = (param1 >> 16) & 0xffff;
			rectangle->top = param1 & 0xffff;
			rectangle->right = (((param2 >> 16) & 0xffff) + rectangle->left) - 1;
			rectangle->bottom = ((param2 & 0xffff) + rectangle->top) - 1;

			if (mod->instance->settings->RefreshRect)
			{
				if (mod->instance->update != NULL)
				{
					if (mod->instance->update->RefreshRect != NULL)
					{
						if (mod->instance->context != NULL)
						{
							LLOGLN(0, ("update rectangle left: %d top: %d bottom: %d right: %d",
											rectangle->left, rectangle->top, rectangle->bottom, rectangle->right));
							mod->instance->update->RefreshRect(mod->instance->context, 1, rectangle);
						}
						else
						{
							LLOGLN(0, ("Invalidate request -The context is null"));
						}
					}
					else
					{
						LLOGLN(0, ("Invalidate request - RefreshRect is Null"));
					}
				}
				else
				{
					LLOGLN(0, ("Invalidate request -the update pointer is null"));
				}
			}
			else
			{
				LLOGLN(0, ("Invalidate request - warning - update rectangle is disabled"));
			}

			free(rectangle);
			break;

		default:
			LLOGLN(0, ("Unhandled message type in eventhandler %d", msg));
			break;
	}

	return 0;
}

static int freerdp_xrdp_client_end(xrdpModule* mod)
{
	int i;
	int j;

	for (j = 0; j < 4; j++)
	{
		for (i = 0; i < 4096; i++)
		{
			free(mod->bitmap_cache[j][i].data);
		}
	}

	for (i = 0; i < 64; i++)
	{
		if (mod->brush_cache[i].data != mod->brush_cache[i].b8x8)
		{
			free(mod->brush_cache[i].data);
		}
	}

	LLOGLN(10, ("freerdp_xrdp_client_end:"));
	return 0;
}

static int freerdp_xrdp_client_set_param(xrdpModule* mod, char *name, char *value)
{
	rdpSettings* settings;

	LLOGLN(10, ("freerdp_xrdp_client_set_param: name [%s] value [%s]", name, value));
	settings = mod->instance->settings;

	LLOGLN(10, ("%p %d", settings->ServerHostname, settings->DisableEncryption));

	if (g_strcmp(name, "hostname") == 0)
	{
	}
	else if (g_strcmp(name, "ip") == 0)
	{
		settings->ServerHostname = g_strdup(value);
	}
	else if (g_strcmp(name, "port") == 0)
	{
		settings->ServerPort = g_atoi(value);
	}
	else if (g_strcmp(name, "keylayout") == 0)
	{
	}
	else if (g_strcmp(name, "name") == 0)
	{
	}
	else if (g_strcmp(name, "lib") == 0)
	{
	}
	else if (g_strcmp(name, "username") == 0)
	{
		g_strncpy(mod->username, value, 255);
	}
	else if (g_strcmp(name, "password") == 0)
	{
		g_strncpy(mod->password, value, 255);
	}
	else if (g_strcasecmp(name, "settings") == 0)
	{
		mod->settings = (rdpSettings*) value;
	}
	else
	{
		LLOGLN(0, ("freerdp_xrdp_client_set_param: unknown name [%s] value [%s]", name, value));
	}

	return 0;
}

int freerdp_xrdp_client_get_event_handles(xrdpModule* mod, HANDLE* events, DWORD* nCount)
{
	return 0;
}

int freerdp_xrdp_client_check_event_handles(xrdpModule* mod)
{
	BOOL status;

	status = freerdp_check_fds(mod->instance);

	if (!status)
		return 1;

	return 0;
}

static void xrdp_freerdp_begin_paint(rdpContext* context)
{
	xrdpModule* mod;

	LLOGLN(10, ("xrdp_freerdp_begin_paint:"));
	mod = ((modContext*) context)->modi;
	mod->server->BeginUpdate(mod);
}

static void xrdp_freerdp_end_paint(rdpContext* context)
{
	xrdpModule* mod;

	LLOGLN(10, ("xrdp_freerdp_end_paint:"));
	mod = ((modContext*) context)->modi;
	mod->server->EndUpdate(mod);
}

static void xrdp_freerdp_set_bounds(rdpContext* context, rdpBounds *bounds)
{
	xrdpModule* mod;
	XRDP_MSG_SET_CLIPPING_REGION msg;

	LLOGLN(10, ("xrdp_freerdp_set_bounds: %p", bounds));
	mod = ((modContext*) context)->modi;

	if (bounds != 0)
	{
		msg.nLeftRect = bounds->left;
		msg.nTopRect = bounds->top;
		msg.nWidth = (bounds->right - bounds->left) + 1;
		msg.nHeight = (bounds->bottom - bounds->top) + 1;

		mod->server->SetClippingRegion(mod, &msg);
	}
	else
	{
		mod->server->SetNullClippingRegion(mod);
	}
}

static void xrdp_freerdp_bitmap_update(rdpContext* context, BITMAP_UPDATE *bitmap)
{
	xrdpModule* mod;
	int index;
	int cx;
	int cy;
	int server_bpp;
	int server_Bpp;
	int client_bpp;
	int j;
	int line_bytes;
	BITMAP_DATA *bd;
	char *dst_data;
	char *dst_data1;
	char *src;
	char *dst;
	XRDP_MSG_PAINT_RECT msg;

	mod = ((modContext*) context)->modi;
	LLOGLN(10, ("xrdp_freerdp_bitmap_update: %d %d", bitmap->number, bitmap->count));

	server_bpp = mod->instance->settings->ColorDepth;
	server_Bpp = (server_bpp + 7) / 8;
	client_bpp = mod->bpp;

	for (index = 0; index < bitmap->number; index++)
	{
		bd = &bitmap->rectangles[index];
		cx = (bd->destRight - bd->destLeft) + 1;
		cy = (bd->destBottom - bd->destTop) + 1;
		line_bytes = server_Bpp * bd->width;
		dst_data = (char *) g_malloc(bd->height * line_bytes + 16, 0);

		if (bd->compressed)
		{
			LLOGLN(20,("decompress size : %d",bd->bitmapLength));
			if (!bitmap_decompress(bd->bitmapDataStream, (BYTE *) dst_data, bd->width, bd->height,
					bd->bitmapLength, server_bpp, server_bpp))
			{
				LLOGLN(0,("Failure to decompress the bitmap"));
			}
		}
		else
		{
			/* bitmap is upside down */
			LLOGLN(10,("bitmap upside down"));
			src = (char *) (bd->bitmapDataStream);
			dst = dst_data + bd->height * line_bytes;

			for (j = 0; j < bd->height; j++)
			{
				dst -= line_bytes;
				g_memcpy(dst, src, line_bytes);
				src += line_bytes;
			}
		}

		dst_data1 = convert_bitmap(server_bpp, client_bpp, dst_data, bd->width, bd->height, mod->colormap);

		msg.nLeftRect = bd->destLeft;
		msg.nTopRect = bd->destTop;
		msg.nWidth = cx;
		msg.nHeight = cy;
		msg.bitmapData = (BYTE*) dst_data1;
		msg.bitmapDataLength = 0;
		msg.nXSrc = 0;
		msg.nYSrc = 0;

		mod->server->PaintRect(mod, &msg);

		if (dst_data1 != dst_data)
		{
			free(dst_data1);
		}

		free(dst_data);
	}
}

static void xrdp_freerdp_dst_blt(rdpContext* context, DSTBLT_ORDER* dstblt)
{
	xrdpModule* mod;
	XRDP_MSG_OPAQUE_RECT msg;

	mod = ((modContext*) context)->modi;
	LLOGLN(10, ("xrdp_freerdp_dst_blt:"));

	msg.nLeftRect = dstblt->nLeftRect;
	msg.nTopRect = dstblt->nTopRect;
	msg.nWidth = dstblt->nWidth;
	msg.nHeight = dstblt->nHeight;

	mod->server->SetRop2(mod, dstblt->bRop);
	mod->server->OpaqueRect(mod, &msg);
	mod->server->SetRop2(mod, 0xcc);
}

static void xrdp_freerdp_pat_blt(rdpContext* context, PATBLT_ORDER* patblt)
{
	xrdpModule* mod;
	int idx;
	int fgcolor;
	int bgcolor;
	int server_bpp;
	int client_bpp;
	struct brush_item* bi;
	XRDP_MSG_PATBLT msg;

	mod = ((modContext*) context)->modi;
	LLOGLN(10, ("xrdp_freerdp_pat_blt:"));

	msg.nLeftRect = patblt->nLeftRect;
	msg.nTopRect = patblt->nTopRect;
	msg.nWidth = patblt->nWidth;
	msg.nHeight = patblt->nHeight;

	server_bpp = mod->instance->settings->ColorDepth;
	client_bpp = mod->bpp;
	LLOGLN(0, ("xrdp_freerdp_pat_blt: bpp %d %d", server_bpp, client_bpp));

	fgcolor = convert_color(server_bpp, client_bpp, patblt->foreColor, mod->colormap);
	bgcolor = convert_color(server_bpp, client_bpp, patblt->backColor, mod->colormap);

	msg.bRop = patblt->bRop;
	msg.foreColor = fgcolor;
	msg.backColor = bgcolor;

	msg.brush.x = patblt->brush.x;
	msg.brush.y = patblt->brush.y;

	if (patblt->brush.style & 0x80)
	{
		idx = patblt->brush.hatch;
		bi = mod->brush_cache + idx;

		msg.brush.style = 3;
		msg.brush.data = (BYTE*) msg.brush.p8x8;
		CopyMemory(msg.brush.data, bi->b8x8, 8);
	}
	else
	{
		msg.brush.style = patblt->brush.style;
		msg.brush.data = (BYTE*) patblt->brush.p8x8;
	}

	mod->server->PatBlt(mod, &msg);
}

static void xrdp_freerdp_scr_blt(rdpContext* context, SCRBLT_ORDER* scrblt)
{
	xrdpModule* mod;
	XRDP_MSG_SCREEN_BLT msg;

	mod = ((modContext*) context)->modi;

	LLOGLN(10, ("xrdp_freerdp_scr_blt:"));

	msg.bRop = scrblt->bRop;
	msg.nLeftRect = scrblt->nLeftRect;
	msg.nTopRect = scrblt->nTopRect;
	msg.nWidth = scrblt->nWidth;
	msg.nHeight = scrblt->nHeight;
	msg.nXSrc = scrblt->nXSrc;
	msg.nYSrc = scrblt->nYSrc;

	mod->server->SetRop2(mod, msg.bRop);
	mod->server->ScreenBlt(mod, &msg);

	mod->server->SetRop2(mod, 0xcc);
}

static void xrdp_freerdp_opaque_rect(rdpContext* context, OPAQUE_RECT_ORDER* opaqueRect)
{
	xrdpModule* mod;
	int server_bpp;
	int client_bpp;
	int fgcolor;
	XRDP_MSG_OPAQUE_RECT msg;

	mod = ((modContext*) context)->modi;
	LLOGLN(10, ("xrdp_freerdp_opaque_rect:"));

	msg.nLeftRect = opaqueRect->nLeftRect;
	msg.nTopRect = opaqueRect->nTopRect;
	msg.nWidth = opaqueRect->nWidth;
	msg.nHeight = opaqueRect->nHeight;

	server_bpp = mod->instance->settings->ColorDepth;
	client_bpp = mod->bpp;

	fgcolor = convert_color(server_bpp, client_bpp, opaqueRect->color, mod->colormap);

	mod->server->SetForeColor(mod, fgcolor);
	mod->server->OpaqueRect(mod, &msg);
}

static void xrdp_freerdp_mem_blt(rdpContext* context, MEMBLT_ORDER* memblt)
{
	int id;
	int idx;
	xrdpModule* mod;
	struct bitmap_item *bi;
	XRDP_MSG_PAINT_RECT msg;

	mod = ((modContext*) context)->modi;
	LLOGLN(10, ("xrdp_freerdp_mem_blt: cacheId %d cacheIndex %d",
					memblt->cacheId, memblt->cacheIndex));

	id = memblt->cacheId;
	idx = memblt->cacheIndex;

	if (idx == 32767) /* BITMAPCACHE_WAITING_LIST_INDEX */
	{
		idx = 4096 - 1;
	}

	if ((id < 0) || (id >= 4))
	{
		LLOGLN(0, ("xrdp_freerdp_mem_blt: bad id [%d]", id));
		return;
	}

	if ((idx < 0) || (idx >= 4096))
	{
		LLOGLN(0, ("xrdp_freerdp_mem_blt: bad idx [%d]", idx));
		return;
	}

	bi = &(mod->bitmap_cache[id][idx]);

	msg.nLeftRect = memblt->nLeftRect;
	msg.nTopRect= memblt->nTopRect;
	msg.nWidth = memblt->nWidth;
	msg.nHeight = memblt->nHeight;
	msg.bitmapData = (BYTE*) bi->data;
	msg.nXSrc = memblt->nXSrc;
	msg.nYSrc = memblt->nYSrc;

	mod->server->SetRop2(mod, memblt->bRop);
	mod->server->PaintRect(mod, &msg);
	mod->server->SetRop2(mod, 0xCC);
}

static void xrdp_freerdp_glyph_index(rdpContext* context, GLYPH_INDEX_ORDER* glyph_index)
{
	xrdpModule* mod;
	int server_bpp;
	int client_bpp;
	int fgcolor;
	int bgcolor;

	mod = ((modContext*) context)->modi;
	LLOGLN(10, ("xrdp_freerdp_glyph_index:"));
	server_bpp = mod->instance->settings->ColorDepth;
	client_bpp = mod->bpp;

	fgcolor = convert_color(server_bpp, client_bpp, glyph_index->foreColor, mod->colormap);
	bgcolor = convert_color(server_bpp, client_bpp, glyph_index->backColor, mod->colormap);

	mod->server->SetBackColor(mod, fgcolor);
	mod->server->SetForeColor(mod, bgcolor);

	mod->server->Text(mod, glyph_index);
}

static void xrdp_freerdp_line_to(rdpContext* context, LINE_TO_ORDER* line_to)
{
	xrdpModule* mod;
	int server_bpp;
	int client_bpp;
	int fgcolor;
	int bgcolor;

	mod = ((modContext*) context)->modi;
	LLOGLN(10, ("xrdp_freerdp_line_to:"));
	mod->server->SetRop2(mod, line_to->bRop2);
	server_bpp = mod->instance->settings->ColorDepth;
	client_bpp = mod->bpp;
	fgcolor = convert_color(server_bpp, client_bpp, line_to->penColor, mod->colormap);
	bgcolor = convert_color(server_bpp, client_bpp, line_to->backColor, mod->colormap);
	mod->server->SetForeColor(mod, fgcolor);
	mod->server->SetBackColor(mod, bgcolor);
	mod->server->SetPen(mod, line_to->penStyle, line_to->penWidth);
	mod->server->LineTo(mod, line_to->nXStart, line_to->nYStart, line_to->nXEnd, line_to->nYEnd);
	mod->server->SetRop2(mod, 0xcc);
}

static void xrdp_freerdp_cache_bitmap(rdpContext* context, CACHE_BITMAP_ORDER* cache_bitmap_order)
{
	LLOGLN(0, ("xrdp_freerdp_cache_bitmap: - no code here"));
}

/* Turn the bitmap upside down*/
static void xrdp_freerdp_upsidedown(UINT8 *destination, CACHE_BITMAP_V2_ORDER* cache_bitmap_v2_order, int server_Bpp)
{
	BYTE *src;
	BYTE *dst;
	int line_bytes;
	int j;

	if (destination == NULL)
	{
		LLOGLN(0, ("xrdp_freerdp_upsidedown: destination pointer is NULL !!!"));
		return;
	}

	line_bytes = server_Bpp * cache_bitmap_v2_order->bitmapWidth;
	src = cache_bitmap_v2_order->bitmapDataStream;
	dst = destination + ((cache_bitmap_v2_order->bitmapHeight) * line_bytes);

	for (j = 0; j < cache_bitmap_v2_order->bitmapHeight; j++)
	{
		dst -= line_bytes;
		g_memcpy(dst, src, line_bytes);
		src += line_bytes;
	}
}

static void xrdp_freerdp_cache_bitmapV2(rdpContext* context, CACHE_BITMAP_V2_ORDER* cache_bitmap_v2_order)
{
	char *dst_data;
	char *dst_data1;
	int bytes;
	int width;
	int height;
	int id;
	int idx;
	int flags;
	int server_bpp;
	int server_Bpp;
	int client_bpp;
	xrdpModule* mod;

	LLOGLN(10, ("xrdp_freerdp_cache_bitmapV2: %d %d 0x%8.8x compressed %d",
					cache_bitmap_v2_order->cacheId,
					cache_bitmap_v2_order->cacheIndex,
					cache_bitmap_v2_order->flags,
					cache_bitmap_v2_order->compressed));

	mod = ((modContext*) context)->modi;
	id = cache_bitmap_v2_order->cacheId;
	idx = cache_bitmap_v2_order->cacheIndex;
	flags = cache_bitmap_v2_order->flags;

	if (flags & 0x10) /* CBR2_DO_NOT_CACHE */
	{
		LLOGLN(0, ("xrdp_freerdp_cache_bitmapV2: CBR2_DO_NOT_CACHE"));
		idx = 4096 - 1;
	}

	if ((id < 0) || (id >= 4))
	{
		LLOGLN(0, ("xrdp_freerdp_cache_bitmapV2: bad id [%d]", id));
		return;
	}

	if ((idx < 0) || (idx >= 4096))
	{
		LLOGLN(0, ("xrdp_freerdp_cache_bitmapV2: bad idx [%d]", idx));
		return;
	}

	server_bpp = mod->instance->settings->ColorDepth;
	server_Bpp = (server_bpp + 7) / 8;
	client_bpp = mod->bpp;

	width = cache_bitmap_v2_order->bitmapWidth;
	height = cache_bitmap_v2_order->bitmapHeight;
	bytes = width * height * server_Bpp + 16;
	dst_data = (char *) g_malloc(bytes, 0);

	if (cache_bitmap_v2_order->compressed)
	{
		bitmap_decompress(cache_bitmap_v2_order->bitmapDataStream, (BYTE *) dst_data, width, height,
				cache_bitmap_v2_order->bitmapLength, server_bpp, server_bpp);
	} else
	{
		/* Uncompressed bitmaps are upside down */
		xrdp_freerdp_upsidedown((BYTE *) dst_data, cache_bitmap_v2_order, server_Bpp);
		LLOGLN(10, ("xrdp_freerdp_cache_bitmapV2:  upside down progressed"));
	}

	dst_data1 = convert_bitmap(server_bpp, client_bpp, dst_data, width, height, mod->colormap);
	free(mod->bitmap_cache[id][idx].data);
	mod->bitmap_cache[id][idx].width = width;
	mod->bitmap_cache[id][idx].height = height;
	mod->bitmap_cache[id][idx].data = dst_data1;

	if (dst_data != dst_data1)
	{
		free(dst_data);
	}
}

static void xrdp_freerdp_cache_glyph(rdpContext* context, CACHE_GLYPH_ORDER* cache_glyph_order)
{
	int index;
	GLYPH_DATA *gd;
	xrdpModule* mod;
	XRDP_MSG_CACHE_GLYPH msg;

	mod = ((modContext*) context)->modi;
	LLOGLN(10, ("xrdp_freerdp_cache_glyph: %d", cache_glyph_order->cGlyphs));

	for (index = 0; index < cache_glyph_order->cGlyphs; index++)
	{
		gd = &cache_glyph_order->glyphData[index];
		LLOGLN(10, ("  %d %d %d %d %d", gd->cacheIndex, gd->x, gd->y, gd->cx, gd->cy));

		msg.flags = 0;
		msg.cGlyphs = 1;
		msg.cacheId = cache_glyph_order->cacheId;
		msg.glyphData[0].cacheIndex = gd->cacheIndex;
		msg.glyphData[0].x = gd->x;
		msg.glyphData[0].y = gd->y;
		msg.glyphData[0].cx = gd->cx;
		msg.glyphData[0].cy = gd->cy;
		msg.glyphData[0].aj = gd->aj;
		msg.glyphData[0].cb = gd->cb;

		mod->server->AddChar(mod, &msg);

		free(gd->aj);
		gd->aj = NULL;
	}

	free(cache_glyph_order->unicodeCharacters);
	cache_glyph_order->unicodeCharacters = 0;
}

static void xrdp_freerdp_cache_brush(rdpContext* context, CACHE_BRUSH_ORDER* cache_brush_order)
{
	int idx;
	int bytes;
	int bpp;
	int cx;
	int cy;
	xrdpModule* mod;

	mod = ((modContext*) context)->modi;
	bpp = cache_brush_order->bpp;
	cx = cache_brush_order->cx;
	cy = cache_brush_order->cy;
	idx = cache_brush_order->index;
	bytes = cache_brush_order->length;
	LLOGLN(10, ("xrdp_freerdp_cache_brush: bpp %d cx %d cy %d idx %d bytes %d",
					bpp, cx, cy, idx, bytes));

	if ((idx < 0) || (idx >= 64))
	{
		LLOGLN(0, ("xrdp_freerdp_cache_brush: error idx %d", idx));
		return;
	}

	if ((bpp != 1) || (cx != 8) || (cy != 8))
	{
		LLOGLN(0, ("xrdp_freerdp_cache_brush: error unsupported brush "
						"bpp %d cx %d cy %d", bpp, cx, cy));
		return;
	}

	mod->brush_cache[idx].bpp = bpp;
	mod->brush_cache[idx].width = cx;
	mod->brush_cache[idx].height = cy;
	mod->brush_cache[idx].data = mod->brush_cache[idx].b8x8;

	if (bytes > 8)
	{
		bytes = 8;
	}

	g_memset(mod->brush_cache[idx].data, 0, 8);

	if (bytes > 0)
	{
		if (bytes > 8)
		{
			LLOGLN(0, ("xrdp_freerdp_cache_brush: bytes to big %d", bytes));
			bytes = 8;
		}

		g_memcpy(mod->brush_cache[idx].data, cache_brush_order->data, bytes);
	}

	LLOGLN(10, ("xrdp_freerdp_cache_brush: out bpp %d cx %d cy %d idx %d bytes %d",
					bpp, cx, cy, idx, bytes));
}

static void xrdp_freerdp_pointer_position(rdpContext* context, POINTER_POSITION_UPDATE *pointer_position)
{
	LLOGLN(0, ("xrdp_freerdp_pointer_position: - no code here"));
}

static void xrdp_freerdp_pointer_system(rdpContext* context, POINTER_SYSTEM_UPDATE *pointer_system)
{
	LLOGLN(0, ("xrdp_freerdp_pointer_system: - no code here type value = %d",pointer_system->type));
}

static void xrdp_freerdp_pointer_color(rdpContext* context, POINTER_COLOR_UPDATE *pointer_color)
{
	LLOGLN(0, ("xrdp_freerdp_pointer_color: - no code here"));
}

static int xrdp_freerdp_get_pixel(void *bits, int width, int height, int bpp, int delta, int x, int y)
{
	int start;
	int shift;
	int pixel;
	BYTE *src8;

	if (bpp == 1)
	{
		src8 = (BYTE *) bits;
		start = (y * delta) + x / 8;
		shift = x % 8;
		pixel = (src8[start] & (0x80 >> shift)) != 0;
		return pixel ? 0xffffff : 0;
	}
	else
	{
		LLOGLN(0, ("xrdp_freerdp_get_pixel: unknown bpp %d", bpp));
	}

	return 0;
}

static int xrdp_freerdp_set_pixel(int pixel, void *bits, int width, int height, int bpp, int delta, int x, int y)
{
	BYTE *dst8;
	int start;
	int shift;

	if (bpp == 1)
	{
		dst8 = (BYTE *) bits;
		start = (y * delta) + x / 8;
		shift = x % 8;

		if (pixel)
		{
			dst8[start] = dst8[start] | (0x80 >> shift);
		}
		else
		{
			dst8[start] = dst8[start] & ~(0x80 >> shift);
		}
	}
	else if (bpp == 24)
	{
		dst8 = (BYTE *) bits;
		dst8 += y * delta + x * 3;
		dst8[0] = (pixel >> 0) & 0xff;
		dst8[1] = (pixel >> 8) & 0xff;
		dst8[2] = (pixel >> 16) & 0xff;
	}
	else
	{
		LLOGLN(0, ("xrdp_freerdp_set_pixel: unknown bpp %d", bpp));
	}

	return 0;
}

static int xrdp_freerdp_convert_color_image(void *dst, int dst_width, int dst_height, int dst_bpp, int dst_delta, void *src,
		int src_width, int src_height, int src_bpp, int src_delta)
{
	int i;
	int j;
	int pixel;

	for (j = 0; j < dst_height; j++)
	{
		for (i = 0; i < dst_width; i++)
		{
			pixel = xrdp_freerdp_get_pixel(src, src_width, src_height, src_bpp, src_delta, i, j);
			xrdp_freerdp_set_pixel(pixel, dst, dst_width, dst_height, dst_bpp, dst_delta, i, j);
		}
	}

	return 0;
}

static void xrdp_freerdp_pointer_new(rdpContext* context, POINTER_NEW_UPDATE* pointer_new)
{
	xrdpModule* mod;
	int index;
	BYTE *dst;
	BYTE *src;
	XRDP_MSG_SET_POINTER msg;

	mod = ((modContext*) context)->modi;
	LLOGLN(20, ("xrdp_freerdp_pointer_new:"));
	LLOGLN(20, ("  bpp %d", pointer_new->xorBpp));
	LLOGLN(20, ("  width %d height %d", pointer_new->colorPtrAttr.width,
					pointer_new->colorPtrAttr.height));

	LLOGLN(20, ("  lengthXorMask %d lengthAndMask %d",
					pointer_new->colorPtrAttr.lengthXorMask,
					pointer_new->colorPtrAttr.lengthAndMask));

	index = pointer_new->colorPtrAttr.cacheIndex;

	if (index >= 32)
	{
		LLOGLN(0,("pointer index too big"));
		return;
	}
	// In this fix we remove the xorBpp check, even if
	// the mouse pointers are not correct we can use them.
	// Configure your destination not to use windows Aero as pointer scheme
	else if ( // pointer_new->xorBpp == 1 &&
		pointer_new->colorPtrAttr.width == 32 && pointer_new->colorPtrAttr.height == 32 && index < 32)
	{
		mod->pointer_cache[index].hotx = pointer_new->colorPtrAttr.xPos;
		mod->pointer_cache[index].hoty = pointer_new->colorPtrAttr.yPos;

		dst = (BYTE *) (mod->pointer_cache[index].data);
		dst += 32 * 32 * 3 - 32 * 3;
		src = pointer_new->colorPtrAttr.xorMaskData;
		xrdp_freerdp_convert_color_image(dst, 32, 32, 24, 32 * -3, src, 32, 32, 1, 32 / 8);

		dst = (BYTE *) (mod->pointer_cache[index].mask);
		dst += (32 * 32 / 8) - (32 / 8);
		src = pointer_new->colorPtrAttr.andMaskData;
		xrdp_freerdp_convert_color_image(dst, 32, 32, 1, 32 / -8, src, 32, 32, 1, 32 / 8);

		msg.xorBpp = 0;
		msg.xPos = mod->pointer_cache[index].hotx;
		msg.yPos = mod->pointer_cache[index].hoty;
		msg.xorMaskData = (BYTE*) mod->pointer_cache[index].data;
		msg.andMaskData = (BYTE*) mod->pointer_cache[index].mask;

		mod->server->SetPointer(mod, &msg);
	}
	else
	{
		LLOGLN(0, ("xrdp_freerdp_pointer_new: error bpp %d width %d height %d index: %d",
						pointer_new->xorBpp, pointer_new->colorPtrAttr.width,
						pointer_new->colorPtrAttr.height,index));
	}

	free(pointer_new->colorPtrAttr.xorMaskData);
	pointer_new->colorPtrAttr.xorMaskData = 0;

	free(pointer_new->colorPtrAttr.andMaskData);
	pointer_new->colorPtrAttr.andMaskData = 0;
}

static void xrdp_freerdp_pointer_cached(rdpContext* context, POINTER_CACHED_UPDATE *pointer_cached)
{
	int index;
	xrdpModule* mod;
	XRDP_MSG_SET_POINTER msg;

	mod = ((modContext*) context)->modi;
	index = pointer_cached->cacheIndex;

	LLOGLN(10, ("xrdp_freerdp_pointer_cached:%d", index));

	msg.xPos = mod->pointer_cache[index].hotx;
	msg.yPos = mod->pointer_cache[index].hoty;
	msg.xorMaskData = (BYTE*) mod->pointer_cache[index].data;
	msg.andMaskData = (BYTE*) mod->pointer_cache[index].mask;

	mod->server->SetPointer(mod, &msg);
}

static void xrdp_freerdp_polygon_cb(rdpContext* context, POLYGON_CB_ORDER* polygon_cb)
{

}

static void xrdp_freerdp_polygon_sc(rdpContext* context, POLYGON_SC_ORDER* polygon_sc)
{
	int i;
	xrdpModule* mod;
	XPoint points[4];
	int fgcolor;
	int server_bpp, client_bpp;

	mod = ((modContext*) context)->modi;
	LLOGLN(10, ("xrdp_freerdp_polygon_sc :%d(points) %d(color) %d(fillmode) %d(bRop) %d(cbData) %d(x) %d(y)", polygon_sc->numPoints,polygon_sc->brushColor,polygon_sc->fillMode,polygon_sc->bRop2,polygon_sc->cbData,polygon_sc->xStart,polygon_sc->yStart));

	if (polygon_sc->numPoints == 3)
	{
		server_bpp = mod->instance->settings->ColorDepth;
		client_bpp = mod->bpp;

		points[0].x = polygon_sc->xStart;
		points[0].y = polygon_sc->yStart;

		for (i = 0; i < polygon_sc->numPoints; i++)
		{
			points[i + 1].x = polygon_sc->points[i].x;
			points[i + 1].y = polygon_sc->points[i].y;
		}
		fgcolor = convert_color(server_bpp, client_bpp, polygon_sc->brushColor, mod->colormap);

		mod->server->SetRop2(mod, polygon_sc->bRop2);
		mod->server->SetBackColor(mod, 255);
		mod->server->SetForeColor(mod, fgcolor);
		mod->server->SetPen(mod, 1, 1);

		// TODO replace with correct brush; this is a workaround
		// This workaround handles the text cursor in microsoft word.
		mod->server->LineTo(mod, polygon_sc->xStart, polygon_sc->yStart, polygon_sc->xStart,
				polygon_sc->yStart + points[2].y);

		mod->server->SetRop2(mod, 0xcc);
	}
	else
	{
		LLOGLN(0, ("Not handled number of points in xrdp_freerdp_polygon_sc"));
	}
}

static void xrdp_freerdp_synchronize(rdpContext* context)
{
	xrdpModule* mod;
	mod = ((modContext*) context)->modi;
	LLOGLN(0, ("xrdp_freerdp_synchronize received - not handled"));
}

static BOOL xrdp_freerdp_pre_connect(freerdp* instance)
{
	xrdpModule* mod;
	rdpUpdate* update;
	rdpSettings* settings;

	LLOGLN(0, ("xrdp_freerdp_pre_connect:"));

	mod = ((modContext*) (instance->context))->modi;
	verifyColorMap(mod);

	settings = instance->settings;
	update = instance->update;

	if (strlen(mod->username) > 0)
		settings->Username = _strdup(mod->username);

	if (strlen(mod->password) > 0)
		settings->Password = _strdup(mod->password);

	settings->OffscreenSupportLevel = FALSE;
	settings->DrawNineGridEnabled = FALSE;
	settings->BitmapCacheEnabled = FALSE;
	settings->GlyphSupportLevel = GLYPH_SUPPORT_FULL;

	CopyMemory(settings->OrderSupport, mod->settings->OrderSupport, 32);

	settings->BitmapCacheV2NumCells = mod->settings->BitmapCacheV2NumCells;

	CopyMemory(settings->BitmapCacheV2CellInfo, mod->settings->BitmapCacheV2CellInfo,
			sizeof(BITMAP_CACHE_V2_CELL_INFO) * settings->BitmapCacheV2NumCells);

	settings->CompressionEnabled = FALSE;
	settings->IgnoreCertificate = TRUE;

	//settings->RemoteApplicationMode = 1;

	if (settings->RemoteApplicationMode)
	{
		settings->RemoteApplicationMode = TRUE;
		settings->RemoteAppLanguageBarSupported = TRUE;
		settings->Workarea = TRUE;
		settings->PerformanceFlags = PERF_DISABLE_WALLPAPER | PERF_DISABLE_FULLWINDOWDRAG;
	}
	else
	{
		settings->PerformanceFlags = PERF_DISABLE_WALLPAPER | PERF_DISABLE_FULLWINDOWDRAG
				| PERF_DISABLE_MENUANIMATIONS | PERF_DISABLE_THEMING;
	}

	update->BeginPaint = xrdp_freerdp_begin_paint;
	update->EndPaint = xrdp_freerdp_end_paint;
	update->SetBounds = xrdp_freerdp_set_bounds;
	update->BitmapUpdate = xrdp_freerdp_bitmap_update;
	update->Synchronize = xrdp_freerdp_synchronize;
	update->primary->DstBlt = xrdp_freerdp_dst_blt;
	update->primary->PatBlt = xrdp_freerdp_pat_blt;
	update->primary->ScrBlt = xrdp_freerdp_scr_blt;
	update->primary->OpaqueRect = xrdp_freerdp_opaque_rect;
	update->primary->MemBlt = xrdp_freerdp_mem_blt;
	update->primary->GlyphIndex = xrdp_freerdp_glyph_index;
	update->primary->LineTo = xrdp_freerdp_line_to;
	update->primary->PolygonSC = xrdp_freerdp_polygon_sc;
	update->primary->PolygonCB = xrdp_freerdp_polygon_cb;
	update->secondary->CacheBitmap = xrdp_freerdp_cache_bitmap;
	update->secondary->CacheBitmapV2 = xrdp_freerdp_cache_bitmapV2;
	update->secondary->CacheGlyph = xrdp_freerdp_cache_glyph;
	update->secondary->CacheBrush = xrdp_freerdp_cache_brush;

	update->pointer->PointerPosition = xrdp_freerdp_pointer_position;
	update->pointer->PointerSystem = xrdp_freerdp_pointer_system;
	update->pointer->PointerColor = xrdp_freerdp_pointer_color;
	update->pointer->PointerNew = xrdp_freerdp_pointer_new;
	update->pointer->PointerCached = xrdp_freerdp_pointer_cached;

	if (settings->Username && settings->Password)
		settings->NlaSecurity = TRUE;
	else
		settings->NlaSecurity = FALSE;

	return TRUE;
}

void lrail_WindowCreate(rdpContext* context, WINDOW_ORDER_INFO* orderInfo, WINDOW_STATE_ORDER* windowState)
{
	xrdpModule* mod;
	XRDP_MSG_WINDOW_NEW_UPDATE msg;

	LLOGLN(0, ("llrail_WindowCreate:"));
	mod = ((modContext*) context)->modi;

	/* copy the window state order */
	msg.ownerWindowId = windowState->ownerWindowId;
	msg.style = windowState->style;
	msg.extendedStyle = windowState->extendedStyle;
	msg.showState = windowState->showState;

	if (orderInfo->fieldFlags & WINDOW_ORDER_FIELD_TITLE)
	{
		ConvertFromUnicode(CP_UTF8, 0, (WCHAR*) windowState->titleInfo.string, windowState->titleInfo.length
				/ 2, (LPSTR*) &msg.titleInfo.string, 0, NULL, NULL);
	}

	msg.clientOffsetX = windowState->clientOffsetX;
	msg.clientOffsetY = windowState->clientOffsetY;
	msg.clientAreaWidth = windowState->clientAreaWidth;
	msg.clientAreaHeight = windowState->clientAreaHeight;
	msg.RPContent = windowState->RPContent;
	msg.rootParentHandle = windowState->rootParentHandle;
	msg.windowOffsetX = windowState->windowOffsetX;
	msg.windowOffsetY = windowState->windowOffsetY;
	msg.windowClientDeltaX = windowState->windowClientDeltaX;
	msg.windowClientDeltaY = windowState->windowClientDeltaY;
	msg.windowWidth = windowState->windowWidth;
	msg.windowHeight = windowState->windowHeight;

	msg.numWindowRects = windowState->numWindowRects;
	msg.windowRects = windowState->windowRects;

	msg.visibleOffsetX = windowState->visibleOffsetX;
	msg.visibleOffsetY = windowState->visibleOffsetY;

	msg.numVisibilityRects = windowState->numVisibilityRects;
	msg.visibilityRects = windowState->visibilityRects;

	mod->server->WindowNewUpdate(mod, &msg);
}

void lrail_WindowUpdate(rdpContext* context, WINDOW_ORDER_INFO* orderInfo, WINDOW_STATE_ORDER* windowState)
{
	LLOGLN(0, ("lrail_WindowUpdate:"));
	lrail_WindowCreate(context, orderInfo, windowState);
}

void lrail_WindowDelete(rdpContext* context, WINDOW_ORDER_INFO* orderInfo)
{
	xrdpModule* mod;
	XRDP_MSG_WINDOW_DELETE msg;

	LLOGLN(0, ("lrail_WindowDelete:"));
	mod = ((modContext*) context)->modi;

	msg.windowId = orderInfo->windowId;

	mod->server->WindowDelete(mod, &msg);
}

void lrail_WindowIcon(rdpContext* context, WINDOW_ORDER_INFO* orderInfo, WINDOW_ICON_ORDER* window_icon)
{
	xrdpModule* mod;
	xrdpRailIconInfo rii;

	LLOGLN(0, ("lrail_WindowIcon:"));
	mod = ((modContext*) context)->modi;
	ZeroMemory(&rii, sizeof(rii));

	rii.bpp = window_icon->iconInfo->bpp;
	rii.width = window_icon->iconInfo->width;
	rii.height = window_icon->iconInfo->height;
	rii.cmap_bytes = window_icon->iconInfo->cbColorTable;
	rii.mask_bytes = window_icon->iconInfo->cbBitsMask;
	rii.data_bytes = window_icon->iconInfo->cbBitsColor;
	rii.mask = (char *) (window_icon->iconInfo->bitsMask);
	rii.cmap = (char *) (window_icon->iconInfo->colorTable);
	rii.data = (char *) (window_icon->iconInfo->bitsColor);

	mod->server->WindowIcon(mod, orderInfo->windowId, window_icon->iconInfo->cacheEntry,
			window_icon->iconInfo->cacheId, &rii, orderInfo->fieldFlags);
}

void lrail_WindowCachedIcon(rdpContext* context, WINDOW_ORDER_INFO* orderInfo, WINDOW_CACHED_ICON_ORDER* window_cached_icon)
{
	xrdpModule* mod;

	LLOGLN(0, ("lrail_WindowCachedIcon:"));
	mod = ((modContext*) context)->modi;

	mod->server->WindowCachedIcon(mod, orderInfo->windowId, window_cached_icon->cachedIcon.cacheEntry,
			window_cached_icon->cachedIcon.cacheId, orderInfo->fieldFlags);
}

void lrail_NotifyIconCreate(rdpContext* context, WINDOW_ORDER_INFO* orderInfo, NOTIFY_ICON_STATE_ORDER* notify_icon_state)
{
	xrdpModule* mod;
	xrdpRailNotifyStateOrder rnso;

	LLOGLN(0, ("lrail_NotifyIconCreate:"));

	mod = ((modContext*) context)->modi;

	ZeroMemory(&rnso, sizeof(rnso));
	rnso.version = notify_icon_state->version;

	if (orderInfo->fieldFlags & WINDOW_ORDER_FIELD_NOTIFY_TIP)
	{
		ConvertFromUnicode(CP_UTF8, 0, (WCHAR*) notify_icon_state->toolTip.string,
				notify_icon_state->toolTip.length / 2, &rnso.tool_tip, 0, NULL, NULL);
	}

	if (orderInfo->fieldFlags & WINDOW_ORDER_FIELD_NOTIFY_INFO_TIP)
	{
		rnso.infotip.timeout = notify_icon_state->infoTip.timeout;
		rnso.infotip.flags = notify_icon_state->infoTip.flags;

		ConvertFromUnicode(CP_UTF8, 0, (WCHAR*) notify_icon_state->infoTip.text.string,
				notify_icon_state->infoTip.text.length / 2, &rnso.infotip.text, 0, NULL, NULL);

		ConvertFromUnicode(CP_UTF8, 0, (WCHAR*) notify_icon_state->infoTip.title.string,
				notify_icon_state->infoTip.title.length / 2, &rnso.infotip.title, 0, NULL, NULL);
	}

	rnso.state = notify_icon_state->state;
	rnso.icon_cache_entry = notify_icon_state->icon.cacheEntry;
	rnso.icon_cache_id = notify_icon_state->icon.cacheId;

	rnso.icon_info.bpp = notify_icon_state->icon.bpp;
	rnso.icon_info.width = notify_icon_state->icon.width;
	rnso.icon_info.height = notify_icon_state->icon.height;
	rnso.icon_info.cmap_bytes = notify_icon_state->icon.cbColorTable;
	rnso.icon_info.mask_bytes = notify_icon_state->icon.cbBitsMask;
	rnso.icon_info.data_bytes = notify_icon_state->icon.cbBitsColor;
	rnso.icon_info.mask = (char*) (notify_icon_state->icon.bitsMask);
	rnso.icon_info.cmap = (char*) (notify_icon_state->icon.colorTable);
	rnso.icon_info.data = (char*) (notify_icon_state->icon.bitsColor);

	mod->server->NotifyNewUpdate(mod, orderInfo->windowId, orderInfo->notifyIconId, &rnso, orderInfo->fieldFlags);

	free(rnso.tool_tip);
	free(rnso.infotip.text);
	free(rnso.infotip.title);
}

void lrail_NotifyIconUpdate(rdpContext* context, WINDOW_ORDER_INFO* orderInfo, NOTIFY_ICON_STATE_ORDER* notify_icon_state)
{
	LLOGLN(0, ("lrail_NotifyIconUpdate:"));
	lrail_NotifyIconCreate(context, orderInfo, notify_icon_state);
}

void lrail_NotifyIconDelete(rdpContext* context, WINDOW_ORDER_INFO* orderInfo)
{
	xrdpModule* mod;

	LLOGLN(0, ("lrail_NotifyIconDelete:"));
	mod = ((modContext*) context)->modi;

	mod->server->NotifyDelete(mod, orderInfo->windowId, orderInfo->notifyIconId);
}

void lrail_MonitoredDesktop(rdpContext* context, WINDOW_ORDER_INFO* orderInfo, MONITORED_DESKTOP_ORDER* monitored_desktop)
{
	int index;
	xrdpModule* mod;
	xrdpRailMonitoredDesktopOrder rmdo;

	LLOGLN(0, ("lrail_MonitoredDesktop:"));
	mod = ((modContext*) context)->modi;

	ZeroMemory(&rmdo, sizeof(rmdo));
	rmdo.active_window_id = monitored_desktop->activeWindowId;
	rmdo.num_window_ids = monitored_desktop->numWindowIds;

	if (orderInfo->fieldFlags & WINDOW_ORDER_FIELD_DESKTOP_ZORDER)
	{
		if (rmdo.num_window_ids > 0)
		{
			rmdo.window_ids = (int *) g_malloc(sizeof(int) * rmdo.num_window_ids, 0);

			for (index = 0; index < rmdo.num_window_ids; index++)
			{
				rmdo.window_ids[index] = monitored_desktop->windowIds[index];
			}
		}
	}

	mod->server->MonitoredDesktop(mod, &rmdo, orderInfo->fieldFlags);

	free(rmdo.window_ids);
}

void lrail_NonMonitoredDesktop(rdpContext* context, WINDOW_ORDER_INFO* orderInfo)
{
	xrdpModule* mod;
	xrdpRailMonitoredDesktopOrder rmdo;

	LLOGLN(0, ("lrail_NonMonitoredDesktop:"));
	mod = ((modContext*) context)->modi;
	ZeroMemory(&rmdo, sizeof(rmdo));

	mod->server->MonitoredDesktop(mod, &rmdo, orderInfo->fieldFlags);
}

static BOOL xrdp_freerdp_post_connect(freerdp* instance)
{
	xrdpModule* mod;

	LLOGLN(0, ("xrdp_freerdp_post_connect:"));
	mod = ((modContext*) (instance->context))->modi;
	g_memset(mod->password, 0, sizeof(mod->password));

	mod->instance->update->window->WindowCreate = lrail_WindowCreate;
	mod->instance->update->window->WindowUpdate = lrail_WindowUpdate;
	mod->instance->update->window->WindowDelete = lrail_WindowDelete;
	mod->instance->update->window->WindowIcon = lrail_WindowIcon;
	mod->instance->update->window->WindowCachedIcon = lrail_WindowCachedIcon;
	mod->instance->update->window->NotifyIconCreate = lrail_NotifyIconCreate;
	mod->instance->update->window->NotifyIconUpdate = lrail_NotifyIconUpdate;
	mod->instance->update->window->NotifyIconDelete = lrail_NotifyIconDelete;
	mod->instance->update->window->MonitoredDesktop = lrail_MonitoredDesktop;
	mod->instance->update->window->NonMonitoredDesktop = lrail_NonMonitoredDesktop;

	return TRUE;
}

static int xrdp_freerdp_context_new(freerdp* instance, rdpContext* context)
{
	LLOGLN(0, ("xrdp_freerdp_context_new: %p", context));
	return 0;
}

static void xrdp_freerdp_context_free(freerdp* instance, rdpContext* context)
{
	LLOGLN(0, ("xrdp_freerdp_context_free: - no code here"));
}

static int xrdp_freerdp_receive_channel_data(freerdp* instance, int channelId, UINT8 *data, int size, int flags, int total_size)
{
	return 0;
}

static BOOL xrdp_freerdp_authenticate(freerdp* instance, char **username, char **password, char **domain)
{
	LLOGLN(0, ("xrdp_freerdp_authenticate: - no code here"));
	return TRUE;
}

static BOOL xrdp_freerdp_verify_certificate(freerdp* instance, char *subject, char *issuer, char *fingerprint)
{
	LLOGLN(0, ("xrdp_freerdp_verify_certificate: - no code here"));
	return TRUE;
}

int freerdp_client_module_init(xrdpModule* mod)
{
	modContext* lcon;
	freerdp* instance;
	xrdpClientModule* client;

	client = (xrdpClientModule*) malloc(sizeof(xrdpClientModule));
	mod->client = client;

	if (client)
	{
		ZeroMemory(client, sizeof(xrdpClientModule));

		client->Connect = freerdp_xrdp_client_connect;
		client->Start = freerdp_xrdp_client_start;
		client->Event = freerdp_xrdp_client_event;
		client->End = freerdp_xrdp_client_end;
		client->SetParam = freerdp_xrdp_client_set_param;
		client->GetEventHandles = freerdp_xrdp_client_get_event_handles;
		client->CheckEventHandles = freerdp_xrdp_client_check_event_handles;
	}

	instance = freerdp_new();
	mod->instance = instance;

	instance->PreConnect = xrdp_freerdp_pre_connect;
	instance->PostConnect = xrdp_freerdp_post_connect;
	instance->ReceiveChannelData = xrdp_freerdp_receive_channel_data;
	instance->Authenticate = xrdp_freerdp_authenticate;
	instance->VerifyCertificate = xrdp_freerdp_verify_certificate;

	instance->ContextSize = sizeof(modContext);
	instance->ContextNew = xrdp_freerdp_context_new;
	instance->ContextFree = xrdp_freerdp_context_free;

	freerdp_context_new(instance);

	lcon = (modContext*) (instance->context);
	lcon->modi = mod;

	return 0;
}

int freerdp_client_module_exit(xrdpModule* mod)
{
	if (!mod)
		return 0;

	if (!mod->instance)
	{
		free(mod);
		return 0;
	}

	freerdp_disconnect(mod->instance);

	freerdp_context_free(mod->instance);

	freerdp_free(mod->instance);
	free(mod);

	return 0;
}
