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

#include <winpr/crt.h>

#include <freerdp/freerdp.h>
#include <freerdp/listener.h>
#include <freerdp/codec/rfx.h>
#include <freerdp/codec/bitmap.h>

#include "core.h"

/**
 * Custom helpers
 */

int libxrdp_set_bounds_rect(xrdpSession* session, xrdpRect* rect)
{
	rdpUpdate* update = session->client->update;

	if (rect)
	{
		rdpBounds bounds;

		bounds.left = rect->left;
		bounds.top = rect->top;
		bounds.right = rect->right - 1;
		bounds.bottom = rect->bottom - 1;

		update->SetBounds(session->context, &bounds);
	}
	else
	{
		update->SetBounds(session->context, NULL);
	}

	return 0;
}

xrdpSession* libxrdp_session_new(rdpSettings* settings)
{
	xrdpSession* session;

	session = (xrdpSession*) malloc(sizeof(xrdpSession));

	if (session)
	{
		ZeroMemory(session, sizeof(xrdpSession));

		session->settings = settings;

		session->bytesPerPixel = 4;

		session->bs = Stream_New(NULL, 16384);
		session->bts = Stream_New(NULL, 16384);

		session->rfx_s = Stream_New(NULL, 16384);
		session->rfx_context = rfx_context_new();

		session->rfx_context->mode = RLGR3;
		session->rfx_context->width = settings->DesktopWidth;
		session->rfx_context->height = settings->DesktopHeight;

		session->nsc_s = Stream_New(NULL, 16384);
		session->nsc_context = nsc_context_new();

		if (session->bytesPerPixel == 4)
		{
			rfx_context_set_pixel_format(session->rfx_context, RDP_PIXEL_FORMAT_B8G8R8A8);
			nsc_context_set_pixel_format(session->nsc_context, RDP_PIXEL_FORMAT_B8G8R8A8);
		}
		else if (session->bytesPerPixel == 3)
		{
			rfx_context_set_pixel_format(session->rfx_context, RDP_PIXEL_FORMAT_B8G8R8);
			nsc_context_set_pixel_format(session->nsc_context, RDP_PIXEL_FORMAT_B8G8R8);
		}
	}

	return session;
}

void libxrdp_session_free(xrdpSession* session)
{
	if (session)
	{
		Stream_Free(session->bs, TRUE);
		Stream_Free(session->bts, TRUE);

		Stream_Free(session->rfx_s, TRUE);
		rfx_context_free(session->rfx_context);

		Stream_Free(session->nsc_s, TRUE);
		nsc_context_free(session->nsc_context);

		free(session);
	}
}

/**
 * Original XRDP stubbed interface
 */

int libxrdp_send_palette(xrdpSession* session, int* palette)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_send_bell(xrdpSession* session)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_send_bitmap(xrdpSession* session, int width, int height, int bpp, char* data, int x, int y, int cx, int cy)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_send_pointer(xrdpSession* session, int cache_idx, char* data, char* mask, int x, int y, int bpp)
{
	POINTER_NEW_UPDATE pointer_new;
	POINTER_COLOR_UPDATE* pointer_color;
	rdpPointerUpdate* pointer = session->client->update->pointer;

	printf("%s\n", __FUNCTION__);

	pointer_color = &(pointer_new.colorPtrAttr);

	pointer_color->cacheIndex = cache_idx;
	pointer_color->xPos = x;
	pointer_color->yPos = y;
	pointer_color->width = 32;
	pointer_color->height = 32;
	pointer_color->lengthAndMask = 128;
	pointer_color->lengthXorMask = 0;
	pointer_color->xorMaskData = (BYTE*) data;
	pointer_color->andMaskData = (BYTE*) mask;

	if (bpp == 0)
	{
		pointer_color->lengthXorMask = 3072;

		IFCALL(pointer->PointerColor, session->context, pointer_color);
	}
	else
	{
		pointer_new.xorBpp = bpp;
		pointer_color->lengthXorMask = ((bpp + 7) / 8) * 32 * 32;

		IFCALL(pointer->PointerNew, session->context, &pointer_new);
	}

	return 0;
}

int libxrdp_set_pointer(xrdpSession* session, int cache_idx)
{
	POINTER_CACHED_UPDATE pointer_cached;
	rdpPointerUpdate* pointer = session->client->update->pointer;

	printf("%s\n", __FUNCTION__);

	pointer_cached.cacheIndex = cache_idx;

	IFCALL(pointer->PointerCached, session->context, &pointer_cached);

	return 0;
}

int libxrdp_orders_init(xrdpSession* session)
{
	rdpUpdate* update = session->context->update;

	printf("%s\n", __FUNCTION__);

	update->BeginPaint(session->context);

	return 0;
}

int libxrdp_orders_send(xrdpSession* session)
{
	rdpUpdate* update = session->context->update;

	printf("%s\n", __FUNCTION__);

	update->EndPaint(session->context);

	return 0;
}

int libxrdp_orders_force_send(xrdpSession* session)
{
	rdpUpdate* update = session->context->update;

	printf("%s\n", __FUNCTION__);

	update->EndPaint(session->context);

	return 0;
}

int libxrdp_orders_rect(xrdpSession* session, int x, int y,
		int cx, int cy, int color, xrdpRect* rect)
{
	OPAQUE_RECT_ORDER opaqueRect;
	rdpPrimaryUpdate* primary = session->client->update->primary;

	printf("%s\n", __FUNCTION__);

	opaqueRect.nLeftRect = x;
	opaqueRect.nTopRect = y;
	opaqueRect.nWidth = cx;
	opaqueRect.nHeight = cy;
	opaqueRect.color = color;

	libxrdp_set_bounds_rect(session, rect);

	IFCALL(primary->OpaqueRect, session->context, &opaqueRect);

	return 0;
}

int libxrdp_orders_screen_blt(xrdpSession* session, int x, int y,
		int cx, int cy, int srcx, int srcy, int rop, xrdpRect* rect)
{
	SCRBLT_ORDER scrblt;
	rdpPrimaryUpdate* primary = session->client->update->primary;

	printf("%s\n", __FUNCTION__);

	scrblt.nLeftRect = x;
	scrblt.nTopRect = y;
	scrblt.nWidth = cx;
	scrblt.nHeight = cy;
	scrblt.bRop = rop;
	scrblt.nXSrc = srcx;
	scrblt.nYSrc = srcy;

	libxrdp_set_bounds_rect(session, rect);

	IFCALL(primary->ScrBlt, session->context, &scrblt);

	return 0;
}

int libxrdp_orders_pat_blt(xrdpSession* session, int x, int y,
		int cx, int cy, int rop, int bg_color, int fg_color,
		xrdpBrush* brush, xrdpRect* rect)
{
	PATBLT_ORDER patblt;
	rdpPrimaryUpdate* primary = session->client->update->primary;

	printf("%s\n", __FUNCTION__);

	patblt.nLeftRect = x;
	patblt.nTopRect = y;
	patblt.nWidth = cx;
	patblt.nHeight = cy;
	patblt.bRop = (UINT32) rop;
	patblt.backColor = (UINT32) fg_color;
	patblt.foreColor = (UINT32) bg_color;

	patblt.brush.x = brush->x_orgin;
	patblt.brush.y = brush->y_orgin;
	patblt.brush.style = brush->style;
	patblt.brush.data = patblt.brush.p8x8;
	CopyMemory(patblt.brush.data, brush->pattern, 8);
	patblt.brush.hatch = patblt.brush.data[0];

	libxrdp_set_bounds_rect(session, rect);

	IFCALL(primary->PatBlt, session->context, &patblt);

	return 0;
}

int libxrdp_orders_dest_blt(xrdpSession* session,
		int x, int y, int cx, int cy, int rop, xrdpRect* rect)
{
	DSTBLT_ORDER dstblt;
	rdpPrimaryUpdate* primary = session->client->update->primary;

	printf("%s\n", __FUNCTION__);

	dstblt.nLeftRect = x;
	dstblt.nTopRect = y;
	dstblt.nWidth = cx;
	dstblt.nHeight = cy;
	dstblt.bRop = rop;

	libxrdp_set_bounds_rect(session, rect);

	IFCALL(primary->DstBlt, session->context, &dstblt);

	return 0;
}

int libxrdp_orders_line(xrdpSession* session, XRDP_MSG_LINE_TO* msg, xrdpRect* rect)
{
	LINE_TO_ORDER lineTo;
	rdpPrimaryUpdate* primary = session->client->update->primary;

	printf("%s\n", __FUNCTION__);

	lineTo.backMode = 1;
	lineTo.nXStart = msg->nXStart;
	lineTo.nYStart = msg->nYStart;
	lineTo.nXEnd = msg->nXEnd;
	lineTo.nYEnd = msg->nYEnd;
	lineTo.backColor = msg->backColor;
	lineTo.bRop2 = msg->bRop2;
	lineTo.penStyle = msg->penStyle;
	lineTo.penWidth = msg->penWidth;
	lineTo.penColor = msg->penColor;

	libxrdp_set_bounds_rect(session, rect);

	IFCALL(primary->LineTo, session->context, &lineTo);

	return 0;
}

int libxrdp_orders_mem_blt(xrdpSession* session, int cache_id,
		int color_table, int x, int y, int cx, int cy, int rop, int srcx,
		int srcy, int cache_idx, xrdpRect* rect)
{
	MEMBLT_ORDER memblt;
	rdpPrimaryUpdate* primary = session->client->update->primary;

	printf("%s id: %d index: %d width: %d height: %d\n",
			__FUNCTION__, cache_id, cache_idx, cx, cy);

	memblt.nLeftRect = x;
	memblt.nTopRect = y;
	memblt.nWidth = cx;
	memblt.nHeight = cy;
	memblt.bRop = rop;
	memblt.nXSrc = srcx;
	memblt.nYSrc = srcy;
	memblt.cacheId = cache_id;
	memblt.cacheIndex = cache_idx;
	memblt.colorIndex = color_table;

	libxrdp_set_bounds_rect(session, rect);

	IFCALL(primary->MemBlt, session->context, &memblt);

	return 0;
}

int libxrdp_orders_text(xrdpSession* session, XRDP_MSG_GLYPH_INDEX* msg, xrdpRect* rect)
{
	GLYPH_INDEX_ORDER glyphIndex;
	rdpPrimaryUpdate* primary = session->client->update->primary;

	printf("%s: cacheId: %d\n", __FUNCTION__, msg->cacheId);

	glyphIndex.backColor = msg->backColor;
	glyphIndex.foreColor = msg->foreColor;
	glyphIndex.cacheId = msg->cacheId;
	glyphIndex.flAccel = msg->flAccel;
	glyphIndex.ulCharInc = msg->ulCharInc;
	glyphIndex.fOpRedundant = msg->fOpRedundant;
	glyphIndex.bkLeft = msg->bkLeft;
	glyphIndex.bkTop = msg->bkTop;
	glyphIndex.bkRight = msg->bkRight;
	glyphIndex.bkBottom = msg->bkBottom;
	glyphIndex.opLeft = msg->opLeft;
	glyphIndex.opTop = msg->opTop;
	glyphIndex.opRight = msg->opRight;
	glyphIndex.opBottom = msg->opBottom;
	CopyMemory(&glyphIndex.brush, &msg->brush, sizeof(rdpBrush));
	glyphIndex.brush.data = glyphIndex.brush.p8x8;
	glyphIndex.x = msg->x;
	glyphIndex.y = msg->y;
	glyphIndex.cbData = msg->cbData;
	CopyMemory(glyphIndex.data, msg->data, glyphIndex.cbData);

	libxrdp_set_bounds_rect(session, rect);

	IFCALL(primary->GlyphIndex, session->context, &glyphIndex);

	return 0;
}

int libxrdp_orders_send_palette(xrdpSession* session, int* palette, int cache_id)
{
	CACHE_COLOR_TABLE_ORDER cache_color_table;
	rdpSecondaryUpdate* secondary = session->client->update->secondary;

	printf("%s\n", __FUNCTION__);

	cache_color_table.cacheIndex = cache_id;
	cache_color_table.numberColors = 256;
	CopyMemory(&(cache_color_table.colorTable), palette, 256 * 4);

	IFCALL(secondary->CacheColorTable, session->context, &cache_color_table);

	return 0;
}

int libxrdp_orders_send_raw_bitmap(xrdpSession* session,
		int width, int height, int bpp, char* data,
		int cache_id, int cache_idx)
{
	int bytesPerPixel;
	CACHE_BITMAP_ORDER cache_bitmap;
	rdpSecondaryUpdate* secondary = session->client->update->secondary;

	printf("%s id: %d index: %d\n", __FUNCTION__, cache_id, cache_idx);

	cache_bitmap.bitmapBpp = bpp;
	cache_bitmap.bitmapWidth = width;
	cache_bitmap.bitmapHeight = height;
	cache_bitmap.bitmapDataStream = (BYTE*) data;
	cache_bitmap.cacheId = cache_id;
	cache_bitmap.cacheIndex = cache_idx;
	cache_bitmap.compressed = FALSE;

	bytesPerPixel = (bpp + 7) / 8;
	cache_bitmap.bitmapLength = width * height * bytesPerPixel;

	IFCALL(secondary->CacheBitmap, session->context, &cache_bitmap);

	return 0;
}

int libxrdp_orders_send_bitmap(xrdpSession* session,
		int width, int height, int bpp, char* data,
		int cache_id, int cache_idx)
{
	wStream* s;
	wStream* ts;
	int e, lines;
	CACHE_BITMAP_ORDER cache_bitmap;
	rdpSecondaryUpdate* secondary = session->client->update->secondary;

	printf("%s id: %d index: %d\n", __FUNCTION__, cache_id, cache_idx);

	e = width % 4;

	if (e != 0)
		e = 4 - e;

	cache_bitmap.bitmapBpp = bpp;
	cache_bitmap.bitmapWidth = width + e;
	cache_bitmap.bitmapHeight = height;
	cache_bitmap.cacheId = cache_id;
	cache_bitmap.cacheIndex = cache_idx;
	cache_bitmap.compressed = TRUE;

	s = session->bs;
	ts = session->bts;

	Stream_SetPosition(s, 0);
	Stream_SetPosition(ts, 0);

	lines = freerdp_bitmap_compress(data, width, height, s, bpp, 16384, height - 1, ts, e);
	Stream_SealLength(s);

	cache_bitmap.bitmapDataStream = Stream_Buffer(s);
	cache_bitmap.bitmapLength = Stream_Length(s);

	IFCALL(secondary->CacheBitmap, session->context, &cache_bitmap);

	return 0;
}

int libxrdp_orders_send_font(xrdpSession* session, XRDP_MSG_CACHE_GLYPH* msg)
{
	rdpSecondaryUpdate* secondary = session->client->update->secondary;

	printf("%s: cacheId: %d cacheIndex: %d\n", __FUNCTION__,
			msg->cacheId, msg->glyphData[0].cacheIndex);

	if (secondary->glyph_v2)
	{
		CACHE_GLYPH_V2_ORDER cache_glyph_v2;

		cache_glyph_v2.flags = 0;
		cache_glyph_v2.cacheId = msg->cacheId;
		cache_glyph_v2.cGlyphs = 1;
		cache_glyph_v2.glyphData[0].cacheIndex = msg->glyphData[0].cacheIndex;
		cache_glyph_v2.glyphData[0].x = msg->glyphData[0].x;
		cache_glyph_v2.glyphData[0].y = msg->glyphData[0].y;
		cache_glyph_v2.glyphData[0].cx = msg->glyphData[0].cx;
		cache_glyph_v2.glyphData[0].cy = msg->glyphData[0].cy;
		cache_glyph_v2.glyphData[0].aj = msg->glyphData[0].aj;
		cache_glyph_v2.unicodeCharacters = NULL;

		IFCALL(secondary->CacheGlyphV2, session->context, &cache_glyph_v2);
	}
	else
	{
		CACHE_GLYPH_ORDER cache_glyph;

		cache_glyph.cacheId = msg->cacheId;
		cache_glyph.cGlyphs = 1;
		cache_glyph.glyphData[0].cacheIndex = msg->glyphData[0].cacheIndex;
		cache_glyph.glyphData[0].x = msg->glyphData[0].x;
		cache_glyph.glyphData[0].y = msg->glyphData[0].y;
		cache_glyph.glyphData[0].cx = msg->glyphData[0].cx;
		cache_glyph.glyphData[0].cy = msg->glyphData[0].cy;
		cache_glyph.glyphData[0].aj = msg->glyphData[0].aj;
		cache_glyph.unicodeCharacters = NULL;

		IFCALL(secondary->CacheGlyph, session->context, &cache_glyph);
	}

	return 0;
}

int libxrdp_reset(xrdpSession* session, XRDP_MSG_RESET* msg)
{
	printf("%s\n", __FUNCTION__);

	session->settings->DesktopWidth = msg->DesktopWidth;
	session->settings->DesktopHeight = msg->DesktopHeight;
	session->settings->ColorDepth = msg->ColorDepth;

	return 0;
}

int libxrdp_orders_send_raw_bitmap2(xrdpSession* session,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx)
{
	int bytesPerPixel;
	CACHE_BITMAP_V2_ORDER cache_bitmap_v2;
	rdpSecondaryUpdate* secondary = session->client->update->secondary;

	printf("%s id: %d index: %d\n", __FUNCTION__, cache_id, cache_idx);

	cache_bitmap_v2.bitmapBpp = bpp;
	cache_bitmap_v2.bitmapWidth = width;
	cache_bitmap_v2.bitmapHeight = height;
	cache_bitmap_v2.bitmapDataStream = (BYTE*) data;
	cache_bitmap_v2.cacheId = cache_id;
	cache_bitmap_v2.cacheIndex = cache_idx;
	cache_bitmap_v2.compressed = FALSE;
	cache_bitmap_v2.flags = 0;

	bytesPerPixel = (bpp + 7) / 8;
	cache_bitmap_v2.cbUncompressedSize = width * height * bytesPerPixel;
	cache_bitmap_v2.bitmapLength = width * height * bytesPerPixel;

	IFCALL(secondary->CacheBitmapV2, session->context, &cache_bitmap_v2);

	return 0;
}

int libxrdp_orders_send_bitmap2(xrdpSession* session,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx, int hints)
{
	wStream* s;
	wStream* ts;
	int e, lines;
	int bytesPerPixel;
	CACHE_BITMAP_V2_ORDER cache_bitmap_v2;
	rdpSecondaryUpdate* secondary = session->client->update->secondary;

	printf("%s id: %d index: %d width: %d height: %d\n",
			__FUNCTION__, cache_id, cache_idx, width, height);

	e = width % 4;

	if (e != 0)
		e = 4 - e;

	cache_bitmap_v2.bitmapBpp = bpp;
	cache_bitmap_v2.bitmapWidth = width + e;
	cache_bitmap_v2.bitmapHeight = height;
	cache_bitmap_v2.cacheId = cache_id;
	cache_bitmap_v2.cacheIndex = cache_idx;
	cache_bitmap_v2.compressed = TRUE;
	cache_bitmap_v2.flags = 0;

	s = session->bs;
	ts = session->bts;

	Stream_SetPosition(s, 0);
	Stream_SetPosition(ts, 0);

	lines = freerdp_bitmap_compress(data, width, height, s, bpp, 16384, height - 1, ts, e);
	Stream_SealLength(s);

	cache_bitmap_v2.bitmapDataStream = Stream_Buffer(s);
	cache_bitmap_v2.bitmapLength = Stream_Length(s);
	cache_bitmap_v2.cbCompMainBodySize = Stream_Length(s);

	bytesPerPixel = (bpp + 7) / 8;
	cache_bitmap_v2.cbUncompressedSize = width * height * bytesPerPixel;

	IFCALL(secondary->CacheBitmapV2, session->context, &cache_bitmap_v2);

	return 0;
}

int libxrdp_orders_send_bitmap3(xrdpSession* session,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx, int hints)
{
	BITMAP_DATA_EX* bitmapData;
	CACHE_BITMAP_V3_ORDER cache_bitmap_v3;
	rdpSecondaryUpdate* secondary = session->client->update->secondary;

	printf("%s\n", __FUNCTION__);

	bitmapData = &(cache_bitmap_v3.bitmapData);

	cache_bitmap_v3.cacheId = cache_id;
	cache_bitmap_v3.bpp = bpp;
	cache_bitmap_v3.flags = 0;
	cache_bitmap_v3.cacheIndex = cache_idx;
	cache_bitmap_v3.key1 = 0;
	cache_bitmap_v3.key2 = 0;

	bitmapData->bpp = 32;
	bitmapData->codecID = 0;
	bitmapData->width = width;
	bitmapData->height = height;
	bitmapData->length = 0;
	bitmapData->data = (BYTE*) data;

	IFCALL(secondary->CacheBitmapV3, session->context, &cache_bitmap_v3);

	return 0;
}

int libxrdp_orders_send_brush(xrdpSession* session, int width, int height,
		int bpp, int type, int size, char* data, int cache_id)
{
	CACHE_BRUSH_ORDER cache_brush;
	rdpSecondaryUpdate* secondary = session->client->update->secondary;

	printf("%s\n", __FUNCTION__);

	cache_brush.index = cache_id;
	cache_brush.bpp = bpp;
	cache_brush.cx = width;
	cache_brush.cy = height;
	cache_brush.style = type;
	cache_brush.length = size;
	CopyMemory(cache_brush.data, data, cache_brush.length);

	IFCALL(secondary->CacheBrush, session->context, &cache_brush);

	return 0;
}

int libxrdp_orders_send_create_os_surface(xrdpSession* session, int id,
		int width, int height, xrdpList* del_list)
{
	int index;
	OFFSCREEN_DELETE_LIST* deleteList;
	CREATE_OFFSCREEN_BITMAP_ORDER create_offscreen_bitmap;
	rdpAltSecUpdate* altsec = session->client->update->altsec;

	printf("%s: id: %d width: %d height: %d\n", __FUNCTION__, id, width, height);

	create_offscreen_bitmap.id = id & 0x7FFF;
	create_offscreen_bitmap.cx = width;
	create_offscreen_bitmap.cy = height;

	deleteList = &(create_offscreen_bitmap.deleteList);
	deleteList->cIndices = deleteList->sIndices = del_list->count;
	deleteList->indices = NULL;

	if (deleteList->cIndices > 0)
	{
		deleteList->indices = (UINT16*) malloc(sizeof(UINT16) * deleteList->cIndices);

		for (index = 0; index < deleteList->cIndices; index++)
		{
			deleteList->indices[index] = list_get_item(del_list, index) & 0x7FFF;
		}
	}

	IFCALL(altsec->CreateOffscreenBitmap, session->context, &create_offscreen_bitmap);

	if (deleteList->indices)
		free(deleteList->indices);

	return 0;
}

int libxrdp_orders_send_switch_os_surface(xrdpSession* session, int id)
{
	SWITCH_SURFACE_ORDER switch_surface;
	rdpAltSecUpdate* altsec = session->client->update->altsec;

	printf("%s: id: %d\n", __FUNCTION__, id);

	switch_surface.bitmapId = id & 0xFFFF;

	IFCALL(altsec->SwitchSurface, session->context, &switch_surface);

	return 0;
}

int libxrdp_send_surface_bits(xrdpSession* session, int bpp, XRDP_MSG_PAINT_RECT* msg)
{
	BYTE* data;
	wStream* s;
	int scanline;
	RFX_RECT rect;
	int bytesPerPixel;
	int MaxRegionWidth;
	int MaxRegionHeight;
	SURFACE_BITS_COMMAND cmd;
	rdpUpdate* update = session->context->update;

	MaxRegionWidth = 64 * 8;
	MaxRegionHeight = 64 * 4;

	if (session->settings->NSCodec)
	{
		MaxRegionWidth = 64 * 5;
		MaxRegionHeight = 64 * 2;
	}

	if ((msg->nWidth * msg->nHeight) > (MaxRegionWidth * MaxRegionHeight))
	{
		int i, j;
		int rows, cols;
		XRDP_MSG_PAINT_RECT subMsg;

		session->frameContext = TRUE;
		libxrdp_orders_send_frame_marker(session, 0, session->frameId);

		rows = (msg->nWidth + (MaxRegionWidth - (msg->nWidth % MaxRegionWidth))) / MaxRegionWidth;
		cols = (msg->nHeight + (MaxRegionHeight - (msg->nHeight % MaxRegionHeight))) / MaxRegionHeight;

		//printf("Partitioning x: %d y: %d width: %d height: %d in %d partitions (%d rows, %d cols)\n",
		//		msg->nLeftRect, msg->nTopRect, msg->nWidth, msg->nHeight, rows * cols, rows, cols);

		for (i = 0; i < rows; i++)
		{
			for (j = 0; j < cols; j++)
			{
				CopyMemory(&subMsg, msg, sizeof(XRDP_MSG_PAINT_RECT));

				subMsg.nLeftRect = msg->nLeftRect + (i * MaxRegionWidth);
				subMsg.nTopRect = msg->nTopRect + (j * MaxRegionHeight);

				subMsg.nWidth = (i < (rows - 1)) ? MaxRegionWidth : msg->nWidth - (i * MaxRegionWidth);
				subMsg.nHeight = (j < (cols - 1)) ? MaxRegionHeight : msg->nHeight - (j * MaxRegionHeight);

				//printf("\t[%d, %d]: x: %d y: %d width: %d height; %d\n",
				//		i, j, subMsg.nLeftRect, subMsg.nTopRect, subMsg.nWidth, subMsg.nHeight);

				if ((subMsg.nWidth * subMsg.nHeight) > 0)
					libxrdp_send_surface_bits(session, bpp, &subMsg);
			}
		}

		libxrdp_orders_send_frame_marker(session, 1, session->frameId++);
		session->frameContext = FALSE;

		return 0;
	}

	if ((bpp == 24) || (bpp == 32))
	{
		bytesPerPixel = 4;
	}
	else
	{
		printf("%s: unsupported bpp: %d\n", __FUNCTION__, bpp);
		return -1;
	}

	if (msg->fbSegmentId)
	{
		bpp = msg->framebuffer->fbBitsPerPixel;

		data = msg->framebuffer->fbSharedMemory;
		data = &data[(msg->nTopRect * msg->framebuffer->fbScanline) +
		             (msg->nLeftRect * msg->framebuffer->fbBytesPerPixel)];

		scanline = msg->framebuffer->fbScanline;
	}
	else
	{
		data = msg->bitmapData;
		scanline = bytesPerPixel * msg->nWidth;
	}

	printf("%s: bpp: %d x: %d y: %d width: %d height: %d\n", __FUNCTION__,
			bpp, msg->nLeftRect, msg->nTopRect, msg->nWidth, msg->nHeight);

	rect.x = 0;
	rect.y = 0;
	rect.width = msg->nWidth;
	rect.height = msg->nHeight;

	if (!session->frameContext)
		libxrdp_orders_send_frame_marker(session, 0, session->frameId);

	if (session->settings->RemoteFxCodec)
	{
		s = session->rfx_s;
		Stream_Clear(s);
		Stream_SetPosition(s, 0);

		rfx_compose_message(session->rfx_context, s, &rect, 1, data,
				msg->nWidth, msg->nHeight, scanline);

		cmd.codecID = session->settings->RemoteFxCodecId;
	}
	else if (session->settings->NSCodec)
	{
		s = session->nsc_s;
		Stream_Clear(s);
		Stream_SetPosition(s, 0);

		nsc_compose_message(session->nsc_context, s, data,
				msg->nWidth, msg->nHeight, scanline);

		cmd.codecID = session->settings->NSCodecId;
	}
	else
	{
		printf("%s: no codecs available!\n", __FUNCTION__);
		return -1;
	}

	cmd.destLeft = msg->nLeftRect;
	cmd.destTop = msg->nTopRect;
	cmd.destRight = msg->nLeftRect + msg->nWidth;
	cmd.destBottom = msg->nTopRect + msg->nHeight;

	cmd.bpp = 32;
	cmd.width = msg->nWidth;
	cmd.height = msg->nHeight;
	cmd.bitmapDataLength = Stream_GetPosition(s);
	cmd.bitmapData = Stream_Buffer(s);

	IFCALL(update->SurfaceBits, update->context, &cmd);

	if (!session->frameContext)
		libxrdp_orders_send_frame_marker(session, 1, session->frameId++);

	return 0;
}

int libxrdp_orders_send_frame_marker(xrdpSession* session, UINT32 action, UINT32 id)
{
	SURFACE_FRAME_MARKER surface_frame_marker;
	rdpUpdate* update = session->client->update;

	printf("%s: action: %d id: %d\n", __FUNCTION__, action, id);

	surface_frame_marker.frameAction = action;
	surface_frame_marker.frameId = id;

	IFCALL(update->SurfaceFrameMarker, session->context, &surface_frame_marker);

	return 0;
}

int libxrdp_window_new_update(xrdpSession* session, XRDP_MSG_WINDOW_NEW_UPDATE* msg)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_window_delete(xrdpSession* session, XRDP_MSG_WINDOW_DELETE* msg)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_window_icon(xrdpSession* session, int window_id,
		int cache_entry, int cache_id, xrdpRailIconInfo* icon_info, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_window_cached_icon(xrdpSession* session, int window_id,
		int cache_entry, int cache_id, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_notify_new_update(xrdpSession* session,
		int window_id, int notify_id, xrdpRailNotifyStateOrder* notify_state, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_notify_delete(xrdpSession* session, int window_id, int notify_id)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int libxrdp_monitored_desktop(xrdpSession* session, xrdpRailMonitoredDesktopOrder* mdo, int flags)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}
