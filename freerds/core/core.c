/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <winpr/crt.h>

#include <freerdp/freerdp.h>
#include <freerdp/listener.h>
#include <freerdp/codec/rfx.h>
#include <freerdp/codec/bitmap.h>

#include "core.h"

/**
 * Custom helpers
 */

int freerds_set_bounds_rect(rdsConnection* connection, rdsRect* rect)
{
	rdpUpdate* update = connection->client->update;

	if (rect)
	{
		rdpBounds bounds;

		bounds.left = rect->left;
		bounds.top = rect->top;
		bounds.right = rect->right - 1;
		bounds.bottom = rect->bottom - 1;

		update->SetBounds((rdpContext*) connection, &bounds);
	}
	else
	{
		update->SetBounds((rdpContext*) connection, NULL);
	}

	return 0;
}

int freerds_send_palette(rdsConnection* connection, int* palette)
{
	//printf("%s\n", __FUNCTION__);
	return 0;
}

int freerds_send_bell(rdsConnection* connection)
{
	//printf("%s\n", __FUNCTION__);
	return 0;
}

int freerds_send_bitmap_update(rdsConnection* connection, int bpp, RDS_MSG_PAINT_RECT* msg)
{
	int nXSrc;
	int nYSrc;
	int nWidth;
	int nHeight;
	BYTE* data;
	BYTE* buffer;
	int yIdx, xIdx, k;
	int rows, cols;
	int nSrcStep;
	BYTE* pSrcData;
	UINT32 DstSize;
	UINT32 SrcFormat;
	BITMAP_DATA* bitmap;
	rdpUpdate* update;
	rdsEncoder* encoder;
	rdpContext* context;
	rdpSettings* settings;
	UINT32 maxUpdateSize;
	UINT32 totalBitmapSize;
	UINT32 updateSizeEstimate;
	BITMAP_DATA* bitmapData;
	BITMAP_UPDATE bitmapUpdate;

	nXSrc = msg->nLeftRect;
	nYSrc = msg->nTopRect;
	nWidth = msg->nWidth;
	nHeight = msg->nHeight;

	context = (rdpContext*) connection;
	update = context->update;

	settings = connection->settings;
	encoder = connection->encoder;

	maxUpdateSize = settings->MultifragMaxRequestSize;

	if (settings->ColorDepth < 32)
		freerds_encoder_prepare(encoder, FREERDP_CODEC_INTERLEAVED);
	else
		freerds_encoder_prepare(encoder, FREERDP_CODEC_PLANAR);

	pSrcData = msg->framebuffer->fbSharedMemory;
	nSrcStep = msg->framebuffer->fbScanline;
	SrcFormat = PIXEL_FORMAT_RGB32;

	if ((nXSrc % 4) != 0)
	{
		nWidth += (nXSrc % 4);
		nXSrc -= (nXSrc % 4);
	}

	if ((nYSrc % 4) != 0)
	{
		nHeight += (nYSrc % 4);
		nYSrc -= (nYSrc % 4);
	}

	rows = (nHeight / 64) + ((nHeight % 64) ? 1 : 0);
	cols = (nWidth / 64) + ((nWidth % 64) ? 1 : 0);

	k = 0;
	totalBitmapSize = 0;

	bitmapUpdate.count = bitmapUpdate.number = rows * cols;
	bitmapData = (BITMAP_DATA*) malloc(sizeof(BITMAP_DATA) * bitmapUpdate.number);
	bitmapUpdate.rectangles = bitmapData;

	if (!bitmapData)
		return -1;

	if ((nWidth % 4) != 0)
	{
		nXSrc -= (nWidth % 4);
		nWidth += (nWidth % 4);
	}

	if ((nHeight % 4) != 0)
	{
		nYSrc -= (nHeight % 4);
		nHeight += (nHeight % 4);
	}

	for (yIdx = 0; yIdx < rows; yIdx++)
	{
		for (xIdx = 0; xIdx < cols; xIdx++)
		{
			bitmap = &bitmapData[k];

			bitmap->width = 64;
			bitmap->height = 64;
			bitmap->destLeft = nXSrc + (xIdx * 64);
			bitmap->destTop = nYSrc + (yIdx * 64);

			if ((bitmap->destLeft + bitmap->width) > (nXSrc + nWidth))
				bitmap->width = (nXSrc + nWidth) - bitmap->destLeft;

			if ((bitmap->destTop + bitmap->height) > (nYSrc + nHeight))
				bitmap->height = (nYSrc + nHeight) - bitmap->destTop;

			bitmap->destRight = bitmap->destLeft + bitmap->width - 1;
			bitmap->destBottom = bitmap->destTop + bitmap->height - 1;
			bitmap->compressed = TRUE;

			if ((bitmap->width < 4) || (bitmap->height < 4))
				continue;

			if (settings->ColorDepth < 32)
			{
				int bitsPerPixel = settings->ColorDepth;
				int bytesPerPixel = (bitsPerPixel + 7) / 8;

				DstSize = 64 * 64 * 4;
				buffer = encoder->grid[k];

				interleaved_compress(encoder->interleaved, buffer, &DstSize, bitmap->width, bitmap->height,
						pSrcData, SrcFormat, nSrcStep, bitmap->destLeft, bitmap->destTop, NULL, bitsPerPixel);

				bitmap->bitmapDataStream = buffer;
				bitmap->bitmapLength = DstSize;
				bitmap->bitsPerPixel = bitsPerPixel;
				bitmap->cbScanWidth = bitmap->width * bytesPerPixel;
				bitmap->cbUncompressedSize = bitmap->width * bitmap->height * bytesPerPixel;
			}
			else
			{
				int dstSize;

				buffer = encoder->grid[k];
				data = &pSrcData[(bitmap->destTop * nSrcStep) + (bitmap->destLeft * 4)];

				buffer = freerdp_bitmap_compress_planar(encoder->planar, data, SrcFormat,
						bitmap->width, bitmap->height, nSrcStep, buffer, &dstSize);

				bitmap->bitmapDataStream = buffer;
				bitmap->bitmapLength = dstSize;
				bitmap->bitsPerPixel = 32;
				bitmap->cbScanWidth = bitmap->width * 4;
				bitmap->cbUncompressedSize = bitmap->width * bitmap->height * 4;
			}

			bitmap->cbCompFirstRowSize = 0;
			bitmap->cbCompMainBodySize = bitmap->bitmapLength;

			totalBitmapSize += bitmap->bitmapLength;
			k++;
		}
	}

	bitmapUpdate.count = bitmapUpdate.number = k;

	updateSizeEstimate = totalBitmapSize + (k * bitmapUpdate.count) + 16;

	if (updateSizeEstimate > maxUpdateSize)
	{
		UINT32 i, j;
		UINT32 updateSize;
		UINT32 newUpdateSize;
		BITMAP_DATA* fragBitmapData;

		fragBitmapData = (BITMAP_DATA*) malloc(sizeof(BITMAP_DATA) * k);
		bitmapUpdate.rectangles = fragBitmapData;

		i = j = 0;
		updateSize = 1024;

		while (i < k)
		{
			newUpdateSize = updateSize + (bitmapData[i].bitmapLength + 16);

			if ((newUpdateSize < maxUpdateSize) && ((i + 1) < k))
			{
				CopyMemory(&fragBitmapData[j++], &bitmapData[i++], sizeof(BITMAP_DATA));
				updateSize = newUpdateSize;
			}
			else
			{
				if ((i + 1) >= k)
				{
					CopyMemory(&fragBitmapData[j++], &bitmapData[i++], sizeof(BITMAP_DATA));
					updateSize = newUpdateSize;
				}

				bitmapUpdate.count = bitmapUpdate.number = j;
				IFCALL(update->BitmapUpdate, context, &bitmapUpdate);
				updateSize = 1024;
				j = 0;
			}
		}

		free(fragBitmapData);
	}
	else
	{
		IFCALL(update->BitmapUpdate, context, &bitmapUpdate);
	}

	free(bitmapData);

	return 1;
}

int freerds_set_pointer(rdsConnection* connection, RDS_MSG_SET_POINTER* msg)
{
	POINTER_NEW_UPDATE pointerNew;
	POINTER_COLOR_UPDATE* pointerColor;
	POINTER_CACHED_UPDATE pointerCached;
	rdpPointerUpdate* pointer = connection->client->update->pointer;

	//printf("%s\n", __FUNCTION__);

	pointerColor = &(pointerNew.colorPtrAttr);

	pointerColor->cacheIndex = 0;
	pointerColor->xPos = msg->xPos;
	pointerColor->yPos = msg->yPos;
	pointerColor->width = 32;
	pointerColor->height = 32;
	pointerColor->lengthAndMask = 128;
	pointerColor->lengthXorMask = 0;
	pointerColor->xorMaskData = msg->xorMaskData;
	pointerColor->andMaskData = msg->andMaskData;

	if (!msg->xorBpp)
	{
		pointerColor->lengthXorMask = 3072;
		IFCALL(pointer->PointerColor, (rdpContext*) connection, pointerColor);
	}
	else
	{
		pointerNew.xorBpp = msg->xorBpp;
		pointerColor->lengthXorMask = ((msg->xorBpp + 7) / 8) * 32 * 32;
		IFCALL(pointer->PointerNew, (rdpContext*) connection, &pointerNew);
	}

	pointerCached.cacheIndex = pointerColor->cacheIndex;

	IFCALL(pointer->PointerCached, (rdpContext*) connection, &pointerCached);

	return 0;
}

int freerds_set_system_pointer(rdsConnection* connection, RDS_MSG_SET_SYSTEM_POINTER* msg)
{
	POINTER_SYSTEM_UPDATE *pointer_system;
	rdpPointerUpdate* pointer = connection->client->update->pointer;

	pointer_system = &(pointer->pointer_system);
	pointer_system->type = msg->ptrType;
	IFCALL(pointer->PointerSystem, (rdpContext *)connection, pointer_system);

	return 0;
}

int freerds_orders_begin_paint(rdsConnection* connection)
{
	rdpUpdate* update = ((rdpContext*) connection)->update;

	//printf("%s\n", __FUNCTION__);

	update->BeginPaint((rdpContext*) connection);

	return 0;
}

int freerds_orders_end_paint(rdsConnection* connection)
{
	rdpUpdate* update = ((rdpContext*) connection)->update;

	//printf("%s\n", __FUNCTION__);

	update->EndPaint((rdpContext*) connection);

	return 0;
}

int freerds_orders_rect(rdsConnection* connection, int x, int y,
		int cx, int cy, int color, rdsRect* rect)
{
	OPAQUE_RECT_ORDER opaqueRect;
	rdpPrimaryUpdate* primary = connection->client->update->primary;

	printf("%s\n", __FUNCTION__);

	opaqueRect.nLeftRect = x;
	opaqueRect.nTopRect = y;
	opaqueRect.nWidth = cx;
	opaqueRect.nHeight = cy;
	opaqueRect.color = color;

	freerds_set_bounds_rect(connection, rect);

	IFCALL(primary->OpaqueRect, (rdpContext*) connection, &opaqueRect);

	return 0;
}

int freerds_orders_screen_blt(rdsConnection* connection, int x, int y,
		int cx, int cy, int srcx, int srcy, int rop, rdsRect* rect)
{
	SCRBLT_ORDER scrblt;
	rdpPrimaryUpdate* primary = connection->client->update->primary;

	//printf("%s\n", __FUNCTION__);

	scrblt.nLeftRect = x;
	scrblt.nTopRect = y;
	scrblt.nWidth = cx;
	scrblt.nHeight = cy;
	scrblt.bRop = rop;
	scrblt.nXSrc = srcx;
	scrblt.nYSrc = srcy;

	freerds_set_bounds_rect(connection, rect);

	IFCALL(primary->ScrBlt, (rdpContext*) connection, &scrblt);

	return 0;
}

int freerds_orders_pat_blt(rdsConnection* connection, int x, int y,
		int cx, int cy, int rop, int bg_color, int fg_color,
		rdpBrush* brush, rdsRect* rect)
{
	PATBLT_ORDER patblt;
	rdpPrimaryUpdate* primary = connection->client->update->primary;

	//printf("%s\n", __FUNCTION__);

	patblt.nLeftRect = x;
	patblt.nTopRect = y;
	patblt.nWidth = cx;
	patblt.nHeight = cy;
	patblt.bRop = (UINT32) rop;
	patblt.backColor = (UINT32) fg_color;
	patblt.foreColor = (UINT32) bg_color;

	patblt.brush.x = brush->x;
	patblt.brush.y = brush->y;
	patblt.brush.style = brush->style;
	patblt.brush.data = patblt.brush.p8x8;
	CopyMemory(patblt.brush.data, brush->data, 8);
	patblt.brush.hatch = patblt.brush.data[0];

	freerds_set_bounds_rect(connection, rect);

	IFCALL(primary->PatBlt, (rdpContext*) connection, &patblt);

	return 0;
}

int freerds_orders_dest_blt(rdsConnection* connection,
		int x, int y, int cx, int cy, int rop, rdsRect* rect)
{
	DSTBLT_ORDER dstblt;
	rdpPrimaryUpdate* primary = connection->client->update->primary;

	//printf("%s\n", __FUNCTION__);

	dstblt.nLeftRect = x;
	dstblt.nTopRect = y;
	dstblt.nWidth = cx;
	dstblt.nHeight = cy;
	dstblt.bRop = rop;

	freerds_set_bounds_rect(connection, rect);

	IFCALL(primary->DstBlt, (rdpContext*) connection, &dstblt);

	return 0;
}

int freerds_orders_line(rdsConnection* connection, RDS_MSG_LINE_TO* msg, rdsRect* rect)
{
	LINE_TO_ORDER lineTo;
	rdpPrimaryUpdate* primary = connection->client->update->primary;

	//printf("%s\n", __FUNCTION__);

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

	freerds_set_bounds_rect(connection, rect);

	IFCALL(primary->LineTo, (rdpContext*) connection, &lineTo);

	return 0;
}

int freerds_orders_mem_blt(rdsConnection* connection, int cache_id,
		int color_table, int x, int y, int cx, int cy, int rop, int srcx,
		int srcy, int cache_idx, rdsRect* rect)
{
	MEMBLT_ORDER memblt;
	rdpPrimaryUpdate* primary = connection->client->update->primary;

	//printf("%s id: %d index: %d width: %d height: %d\n",
	//		__FUNCTION__, cache_id, cache_idx, cx, cy);

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

	freerds_set_bounds_rect(connection, rect);

	IFCALL(primary->MemBlt, (rdpContext*) connection, &memblt);

	return 0;
}

int freerds_orders_text(rdsConnection* connection, RDS_MSG_GLYPH_INDEX* msg, rdsRect* rect)
{
	GLYPH_INDEX_ORDER glyphIndex;
	rdpPrimaryUpdate* primary = connection->client->update->primary;

	//printf("%s: cacheId: %d\n", __FUNCTION__, msg->cacheId);

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

	freerds_set_bounds_rect(connection, rect);

	IFCALL(primary->GlyphIndex, (rdpContext*) connection, &glyphIndex);

	return 0;
}

int freerds_orders_send_palette(rdsConnection* connection, int* palette, int cache_id)
{
	CACHE_COLOR_TABLE_ORDER cache_color_table;
	rdpSecondaryUpdate* secondary = connection->client->update->secondary;

	//printf("%s\n", __FUNCTION__);

	cache_color_table.cacheIndex = cache_id;
	cache_color_table.numberColors = 256;
	CopyMemory(&(cache_color_table.colorTable), palette, 256 * 4);

	IFCALL(secondary->CacheColorTable, (rdpContext*) connection, &cache_color_table);

	return 0;
}

int freerds_orders_send_font(rdsConnection* connection, RDS_MSG_CACHE_GLYPH* msg)
{
	rdpSecondaryUpdate* secondary = connection->client->update->secondary;

	//printf("%s: cacheId: %d cacheIndex: %d\n", __FUNCTION__,
	//		msg->cacheId, msg->glyphData[0].cacheIndex);

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

		IFCALL(secondary->CacheGlyphV2, (rdpContext*) connection, &cache_glyph_v2);
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

		IFCALL(secondary->CacheGlyph, (rdpContext*) connection, &cache_glyph);
	}

	return 0;
}

int freerds_reset(rdsConnection* connection, RDS_MSG_RESET* msg)
{
	//printf("%s\n", __FUNCTION__);

	connection->settings->DesktopWidth = msg->DesktopWidth;
	connection->settings->DesktopHeight = msg->DesktopHeight;
	connection->settings->ColorDepth = msg->ColorDepth;

	return 0;
}

int freerds_orders_send_brush(rdsConnection* connection, int width, int height,
		int bpp, int type, int size, char* data, int cache_id)
{
	CACHE_BRUSH_ORDER cache_brush;
	rdpSecondaryUpdate* secondary = connection->client->update->secondary;

	//printf("%s\n", __FUNCTION__);

	cache_brush.index = cache_id;
	cache_brush.bpp = bpp;
	cache_brush.cx = width;
	cache_brush.cy = height;
	cache_brush.style = type;
	cache_brush.length = size;
	CopyMemory(cache_brush.data, data, cache_brush.length);

	IFCALL(secondary->CacheBrush, (rdpContext*) connection, &cache_brush);

	return 0;
}

int freerds_orders_send_create_os_surface(rdsConnection* connection, CREATE_OFFSCREEN_BITMAP_ORDER* createOffscreenBitmap)
{
	rdpAltSecUpdate* altsec = connection->client->update->altsec;

	//printf("%s: id: %d width: %d height: %d\n", __FUNCTION__, id, width, height);

	IFCALL(altsec->CreateOffscreenBitmap, (rdpContext*) connection, createOffscreenBitmap);

	return 0;
}

int freerds_orders_send_switch_os_surface(rdsConnection* connection, int id)
{
	SWITCH_SURFACE_ORDER switch_surface;
	rdpAltSecUpdate* altsec = connection->client->update->altsec;

	//printf("%s: id: %d\n", __FUNCTION__, id);

	switch_surface.bitmapId = id & 0xFFFF;

	IFCALL(altsec->SwitchSurface, (rdpContext*) connection, &switch_surface);

	return 0;
}

int freerds_send_surface_bits(rdsConnection* connection, int bpp, RDS_MSG_PAINT_RECT* msg)
{
	int i;
	BOOL first;
	BOOL last;
	int nXSrc;
	int nYSrc;
	int nWidth;
	int nHeight;
	wStream* s;
	int nSrcStep;
	BYTE* pSrcData;
	int numMessages;
	UINT32 frameId = 0;
	rdpUpdate* update;
	rdpContext* context;
	rdpSettings* settings;
	rdsEncoder* encoder;
	SURFACE_BITS_COMMAND cmd;

	nXSrc = msg->nLeftRect;
	nYSrc = msg->nTopRect;
	nWidth = msg->nWidth;
	nHeight = msg->nHeight;

	context = (rdpContext*) connection;
	update = context->update;

	settings = connection->settings;
	encoder = connection->encoder;

	pSrcData = msg->framebuffer->fbSharedMemory;
	nSrcStep = msg->framebuffer->fbScanline;

	if (encoder->frameAck)
		frameId = (UINT32) freerds_encoder_create_frame_id(encoder);

	if (settings->RemoteFxCodec)
	{
		RFX_RECT rect;
		RFX_MESSAGE* messages;

		freerds_encoder_prepare(encoder, FREERDP_CODEC_REMOTEFX);

		s = encoder->bs;

		rect.x = nXSrc;
		rect.y = nYSrc;
		rect.width = nWidth;
		rect.height = nHeight;

		messages = rfx_encode_messages(encoder->rfx, &rect, 1, pSrcData,
				msg->framebuffer->fbWidth, msg->framebuffer->fbHeight,
				nSrcStep, &numMessages, settings->MultifragMaxRequestSize);

		cmd.codecID = settings->RemoteFxCodecId;

		cmd.destLeft = 0;
		cmd.destTop = 0;
		cmd.destRight = msg->framebuffer->fbWidth;
		cmd.destBottom = msg->framebuffer->fbHeight;

		cmd.bpp = 32;
		cmd.width = msg->framebuffer->fbWidth;
		cmd.height = msg->framebuffer->fbHeight;

		for (i = 0; i < numMessages; i++)
		{
			Stream_SetPosition(s, 0);
			rfx_write_message(encoder->rfx, s, &messages[i]);
			rfx_message_free(encoder->rfx, &messages[i]);

			cmd.bitmapDataLength = Stream_GetPosition(s);
			cmd.bitmapData = Stream_Buffer(s);

			first = (i == 0) ? TRUE : FALSE;
			last = ((i + 1) == numMessages) ? TRUE : FALSE;

			if (!encoder->frameAck)
				IFCALL(update->SurfaceBits, update->context, &cmd);
			else
				IFCALL(update->SurfaceFrameBits, update->context, &cmd, first, last, frameId);
		}

		free(messages);
	}
	else if (settings->NSCodec)
	{
		freerds_encoder_prepare(encoder, FREERDP_CODEC_NSCODEC);

		s = encoder->bs;
		Stream_SetPosition(s, 0);

		pSrcData = &pSrcData[(nYSrc * nSrcStep) + (nXSrc * 4)];

		nsc_compose_message(encoder->nsc, s, pSrcData, nWidth, nHeight, nSrcStep);

		cmd.bpp = 32;
		cmd.codecID = settings->NSCodecId;
		cmd.destLeft = nXSrc;
		cmd.destTop = nYSrc;
		cmd.destRight = cmd.destLeft + nWidth;
		cmd.destBottom = cmd.destTop + nHeight;
		cmd.width = nWidth;
		cmd.height = nHeight;

		cmd.bitmapDataLength = Stream_GetPosition(s);
		cmd.bitmapData = Stream_Buffer(s);

		first = TRUE;
		last = TRUE;

		if (!encoder->frameAck)
			IFCALL(update->SurfaceBits, update->context, &cmd);
		else
			IFCALL(update->SurfaceFrameBits, update->context, &cmd, first, last, frameId);
	}

	return 1;
}

int freerds_orders_send_frame_marker(rdsConnection* connection, UINT32 action, UINT32 id)
{
	SURFACE_FRAME_MARKER surfaceFrameMarker;
	rdpUpdate* update = connection->client->update;

	//printf("%s: action: %d id: %d\n", __FUNCTION__, action, id);

	surfaceFrameMarker.frameAction = action;
	surfaceFrameMarker.frameId = id;

	IFCALL(update->SurfaceFrameMarker, (rdpContext*) connection, &surfaceFrameMarker);

	return 0;
}

int freerds_window_new_update(rdsConnection* connection, RDS_MSG_WINDOW_NEW_UPDATE* msg)
{
	//printf("%s\n", __FUNCTION__);
	return 0;
}

int freerds_window_delete(rdsConnection* connection, RDS_MSG_WINDOW_DELETE* msg)
{
	//printf("%s\n", __FUNCTION__);
	return 0;
}
