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
#include "app_context.h"

#include <pixman.h>

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

int freerds_connection_init(rdsConnection* connection, rdpSettings* settings)
{
	connection->id = app_context_get_connectionid();
	connection->settings = settings;

	connection->bytesPerPixel = 4;

	connection->encoder = freerds_bitmap_encoder_new(settings->DesktopWidth,
			settings->DesktopHeight, settings->ColorDepth);

	connection->FrameList = ListDictionary_New(TRUE);

	return 0;
}

void freerds_connection_uninit(rdsConnection* connection)
{
	freerds_bitmap_encoder_free(connection->encoder);

	ListDictionary_Free(connection->FrameList);
}

/**
 * Original XRDP stubbed interface
 */

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
	BYTE* data;
	BYTE* tile;
	BYTE* buffer;
	int i, j, k;
	wStream* s;
	wStream* ts;
	int e, lines;
	int scanline;
	int rows, cols;
	int MaxRegionWidth;
	int MaxRegionHeight;
	INT32 nWidth, nHeight;
	pixman_image_t* image;
	pixman_image_t* fbImage;
	BITMAP_DATA* bitmapData;
	BITMAP_UPDATE bitmapUpdate;
	rdpUpdate* update = connection->client->update;

	//printf("%s\n", __FUNCTION__);

	MaxRegionWidth = 64 * 4;
	MaxRegionHeight = 64 * 1;

	if ((msg->nLeftRect % 4) != 0)
	{
		msg->nWidth += (msg->nLeftRect % 4);
		msg->nLeftRect -= (msg->nLeftRect % 4);
	}

	if ((msg->nTopRect % 4) != 0)
	{
		msg->nHeight += (msg->nTopRect % 4);
		msg->nTopRect -= (msg->nTopRect % 4);
	}

	if ((msg->nWidth * msg->nHeight) > (MaxRegionWidth * MaxRegionHeight))
	{
		RDS_MSG_PAINT_RECT subMsg;

		rows = (msg->nWidth + (MaxRegionWidth - (msg->nWidth % MaxRegionWidth))) / MaxRegionWidth;
		cols = (msg->nHeight + (MaxRegionHeight - (msg->nHeight % MaxRegionHeight))) / MaxRegionHeight;

		//printf("Partitioning x: %d y: %d width: %d height: %d in %d partitions (%d rows, %d cols)\n",
		//		msg->nLeftRect, msg->nTopRect, msg->nWidth, msg->nHeight, rows * cols, rows, cols);

		for (i = 0; i < rows; i++)
		{
			for (j = 0; j < cols; j++)
			{
				CopyMemory(&subMsg, msg, sizeof(RDS_MSG_PAINT_RECT));

				subMsg.nLeftRect = msg->nLeftRect + (i * MaxRegionWidth);
				subMsg.nTopRect = msg->nTopRect + (j * MaxRegionHeight);

				subMsg.nWidth = (i < (rows - 1)) ? MaxRegionWidth : msg->nWidth - (i * MaxRegionWidth);
				subMsg.nHeight = (j < (cols - 1)) ? MaxRegionHeight : msg->nHeight - (j * MaxRegionHeight);

				//printf("\t[%d, %d]: x: %d y: %d width: %d height: %d\n",
				//		i, j, subMsg.nLeftRect, subMsg.nTopRect, subMsg.nWidth, subMsg.nHeight);

				if ((subMsg.nWidth * subMsg.nHeight) > 0)
					freerds_send_bitmap_update(connection, bpp, &subMsg);
			}
		}

		return 0;
	}

	tile = (BYTE*) malloc(64 * 64 * 4);
	fbImage = (pixman_image_t*) msg->framebuffer->image;

	rows = (msg->nWidth + (64 - (msg->nWidth % 64))) / 64;
	cols = (msg->nHeight + (64 - (msg->nHeight % 64))) / 64;

	k = 0;
	bitmapUpdate.count = bitmapUpdate.number = rows * cols;
	bitmapData = (BITMAP_DATA*) malloc(sizeof(BITMAP_DATA) * bitmapUpdate.number);
	bitmapUpdate.rectangles = bitmapData;

	if ((msg->nWidth % 4) != 0)
	{
		msg->nLeftRect -= (msg->nWidth % 4);
		msg->nWidth += (msg->nWidth % 4);
	}

	if ((msg->nHeight % 4) != 0)
	{
		msg->nTopRect -= (msg->nHeight % 4);
		msg->nHeight += (msg->nHeight % 4);
	}

	for (i = 0; i < rows; i++)
	{
		for (j = 0; j < cols; j++)
		{
			nWidth = (i < (rows - 1)) ? 64 : msg->nWidth - (i * 64);
			nHeight = (j < (cols - 1)) ? 64 : msg->nHeight - (j * 64);

			bitmapData[k].bitsPerPixel = 16;
			bitmapData[k].width = nWidth;
			bitmapData[k].height = nHeight;
			bitmapData[k].destLeft = msg->nLeftRect + (i * 64);
			bitmapData[k].destTop = msg->nTopRect + (j * 64);
			bitmapData[k].destRight = bitmapData[k].destLeft + nWidth - 1;
			bitmapData[k].destBottom = bitmapData[k].destTop + nHeight - 1;
			bitmapData[k].compressed = TRUE;

			if (((nWidth * nHeight) > 0) && (nWidth >= 4) && (nHeight >= 4))
			{
				e = nWidth % 4;

				if (e != 0)
					e = 4 - e;

				//printf("k: %d destLeft: %d destTop: %d destRight: %d destBottom: %d nWidth: %d nHeight: %d\n",
				//		k, bitmapData[k].destLeft, bitmapData[k].destTop,
				//		bitmapData[k].destRight, bitmapData[k].destBottom, nWidth, nHeight);

				s = connection->encoder->bs;
				ts = connection->encoder->bts;

				Stream_SetPosition(s, 0);
				Stream_SetPosition(ts, 0);

				data = msg->framebuffer->fbSharedMemory;
				data = &data[(bitmapData[k].destTop * msg->framebuffer->fbScanline) +
				             (bitmapData[k].destLeft * msg->framebuffer->fbBytesPerPixel)];

				scanline = msg->framebuffer->fbScanline;

				if (bpp > 16)
				{
					int dstSize;
					UINT32 format;

					format = FREERDP_PIXEL_FORMAT(msg->framebuffer->fbBitsPerPixel,
							FREERDP_PIXEL_FORMAT_TYPE_ARGB, FREERDP_PIXEL_FLIP_NONE);

					buffer = connection->encoder->grid[k];

					buffer = freerdp_bitmap_compress_planar(connection->encoder->planar_context,
							data, format, nWidth, nHeight, scanline, buffer, &dstSize);

					bitmapData[k].bitmapDataStream = buffer;
					bitmapData[k].bitmapLength = dstSize;

					bitmapData[k].bitsPerPixel = 32;
					bitmapData[k].cbScanWidth = nWidth * 4;
					bitmapData[k].cbUncompressedSize = nWidth * nHeight * 4;
				}
				else
				{
					image = pixman_image_create_bits(PIXMAN_r5g6b5, nWidth, nHeight, (uint32_t*) tile, nWidth * 2);

					pixman_image_composite(PIXMAN_OP_OVER, fbImage, NULL, image,
							bitmapData[k].destLeft, bitmapData[k].destTop, 0, 0, 0, 0, nWidth, nHeight);

					lines = freerdp_bitmap_compress((char*) pixman_image_get_data(image),
							nWidth, nHeight, s, 16, 16384, nHeight - 1, ts, e);
					Stream_SealLength(s);

					bitmapData[k].bitmapDataStream = Stream_Buffer(s);
					bitmapData[k].bitmapLength = Stream_Length(s);

					buffer = connection->encoder->grid[k];
					CopyMemory(buffer, bitmapData[k].bitmapDataStream, bitmapData[k].bitmapLength);
					bitmapData[k].bitmapDataStream = buffer;

					bitmapData[k].bitsPerPixel = 16;
					bitmapData[k].cbScanWidth = nWidth * 2;
					bitmapData[k].cbUncompressedSize = nWidth * nHeight * 2;

					pixman_image_unref(image);
				}

				bitmapData[k].cbCompFirstRowSize = 0;
				bitmapData[k].cbCompMainBodySize = bitmapData[k].bitmapLength;

				k++;
			}
		}
	}

	bitmapUpdate.count = bitmapUpdate.number = k;

	IFCALL(update->BitmapUpdate, (rdpContext*) connection, &bitmapUpdate);

	free(bitmapData);
	free(tile);

	return 0;
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

int freerds_orders_send_raw_bitmap(rdsConnection* connection,
		int width, int height, int bpp, char* data,
		int cache_id, int cache_idx)
{
	int bytesPerPixel;
	CACHE_BITMAP_ORDER cache_bitmap;
	rdpSecondaryUpdate* secondary = connection->client->update->secondary;

	//printf("%s id: %d index: %d\n", __FUNCTION__, cache_id, cache_idx);

	cache_bitmap.bitmapBpp = bpp;
	cache_bitmap.bitmapWidth = width;
	cache_bitmap.bitmapHeight = height;
	cache_bitmap.bitmapDataStream = (BYTE*) data;
	cache_bitmap.cacheId = cache_id;
	cache_bitmap.cacheIndex = cache_idx;
	cache_bitmap.compressed = FALSE;

	bytesPerPixel = (bpp + 7) / 8;
	cache_bitmap.bitmapLength = width * height * bytesPerPixel;

	IFCALL(secondary->CacheBitmap, (rdpContext*) connection, &cache_bitmap);

	return 0;
}

int freerds_orders_send_bitmap(rdsConnection* connection,
		int width, int height, int bpp, char* data,
		int cache_id, int cache_idx)
{
	wStream* s;
	wStream* ts;
	int e, lines;
	CACHE_BITMAP_ORDER cache_bitmap;
	rdpSecondaryUpdate* secondary = connection->client->update->secondary;

	//printf("%s id: %d index: %d\n", __FUNCTION__, cache_id, cache_idx);

	e = width % 4;

	if (e != 0)
		e = 4 - e;

	cache_bitmap.bitmapBpp = bpp;
	cache_bitmap.bitmapWidth = width + e;
	cache_bitmap.bitmapHeight = height;
	cache_bitmap.cacheId = cache_id;
	cache_bitmap.cacheIndex = cache_idx;
	cache_bitmap.compressed = TRUE;

	s = connection->encoder->bs;
	ts = connection->encoder->bts;

	Stream_SetPosition(s, 0);
	Stream_SetPosition(ts, 0);

	lines = freerdp_bitmap_compress(data, width, height, s, bpp, 16384, height - 1, ts, e);
	Stream_SealLength(s);

	cache_bitmap.bitmapDataStream = Stream_Buffer(s);
	cache_bitmap.bitmapLength = Stream_Length(s);

	IFCALL(secondary->CacheBitmap, (rdpContext*) connection, &cache_bitmap);

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

int freerds_orders_send_raw_bitmap2(rdsConnection* connection,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx)
{
	int bytesPerPixel;
	CACHE_BITMAP_V2_ORDER cache_bitmap_v2;
	rdpSecondaryUpdate* secondary = connection->client->update->secondary;

	//printf("%s id: %d index: %d\n", __FUNCTION__, cache_id, cache_idx);

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

	IFCALL(secondary->CacheBitmapV2, (rdpContext*) connection, &cache_bitmap_v2);

	return 0;
}

int freerds_orders_send_bitmap2(rdsConnection* connection,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx, int hints)
{
	wStream* s;
	wStream* ts;
	int e, lines;
	int bytesPerPixel;
	CACHE_BITMAP_V2_ORDER cache_bitmap_v2;
	rdpSecondaryUpdate* secondary = connection->client->update->secondary;

	//printf("%s id: %d index: %d width: %d height: %d\n",
	//		__FUNCTION__, cache_id, cache_idx, width, height);

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

	s = connection->encoder->bs;
	ts = connection->encoder->bts;

	Stream_SetPosition(s, 0);
	Stream_SetPosition(ts, 0);

	lines = freerdp_bitmap_compress(data, width, height, s, bpp, 16384, height - 1, ts, e);
	Stream_SealLength(s);

	cache_bitmap_v2.bitmapDataStream = Stream_Buffer(s);
	cache_bitmap_v2.bitmapLength = Stream_Length(s);
	cache_bitmap_v2.cbCompMainBodySize = Stream_Length(s);

	bytesPerPixel = (bpp + 7) / 8;
	cache_bitmap_v2.cbUncompressedSize = width * height * bytesPerPixel;

	IFCALL(secondary->CacheBitmapV2, (rdpContext*) connection, &cache_bitmap_v2);

	return 0;
}

int freerds_orders_send_bitmap3(rdsConnection* connection,
		int width, int height, int bpp, char* data, int cache_id, int cache_idx, int hints)
{
	BITMAP_DATA_EX* bitmapData;
	CACHE_BITMAP_V3_ORDER cache_bitmap_v3;
	rdpSecondaryUpdate* secondary = connection->client->update->secondary;

	//printf("%s\n", __FUNCTION__);

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

	IFCALL(secondary->CacheBitmapV3, (rdpContext*) connection, &cache_bitmap_v3);

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
	BYTE* data;
	wStream* s;
	int scanline;
	int numMessages;
	int bytesPerPixel;
	SURFACE_BITS_COMMAND cmd;
	rdpUpdate* update = ((rdpContext*) connection)->update;

	if (!msg->framebuffer->fbAttached)
		return 0;

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
		scanline = msg->framebuffer->fbScanline;
	}
	else
	{
		data = msg->bitmapData;
		scanline = bytesPerPixel * msg->nWidth;
	}

	printf("%s: bpp: %d x: %d y: %d width: %d height: %d\n", __FUNCTION__,
			bpp, msg->nLeftRect, msg->nTopRect, msg->nWidth, msg->nHeight);

	if (connection->settings->RemoteFxCodec)
	{
		RFX_RECT rect;
		RFX_MESSAGE* messages;

		s = connection->encoder->rfx_s;

		rect.x = msg->nLeftRect;
		rect.y = msg->nTopRect;
		rect.width = msg->nWidth;
		rect.height = msg->nHeight;

		messages = rfx_encode_messages(connection->encoder->rfx_context, &rect, 1, data,
				msg->framebuffer->fbWidth, msg->framebuffer->fbHeight, scanline, &numMessages,
				connection->settings->MultifragMaxRequestSize);

		cmd.codecID = connection->settings->RemoteFxCodecId;

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
			rfx_write_message(connection->encoder->rfx_context, s, &messages[i]);
			rfx_message_free(connection->encoder->rfx_context, &messages[i]);

			cmd.bitmapDataLength = Stream_GetPosition(s);
			cmd.bitmapData = Stream_Buffer(s);

			IFCALL(update->SurfaceBits, update->context, &cmd);
		}

		free(messages);

		return 0;
	}
	else if (connection->settings->NSCodec)
	{
		NSC_MESSAGE* messages;

		s = connection->encoder->nsc_s;

		messages = nsc_encode_messages(connection->encoder->nsc_context, data,
				msg->nLeftRect, msg->nTopRect, msg->nWidth, msg->nHeight,
				scanline, &numMessages, connection->settings->MultifragMaxRequestSize);

		cmd.bpp = 32;
		cmd.codecID = connection->settings->NSCodecId;

		for (i = 0; i < numMessages; i++)
		{
			Stream_SetPosition(s, 0);

			nsc_write_message(connection->encoder->nsc_context, s, &messages[i]);
			nsc_message_free(connection->encoder->nsc_context, &messages[i]);

			cmd.destLeft = messages[i].x;
			cmd.destTop = messages[i].y;
			cmd.destRight = messages[i].x + messages[i].width;
			cmd.destBottom = messages[i].y + messages[i].height;
			cmd.width = messages[i].width;
			cmd.height = messages[i].height;

			cmd.bitmapDataLength = Stream_GetPosition(s);
			cmd.bitmapData = Stream_Buffer(s);

			IFCALL(update->SurfaceBits, update->context, &cmd);
		}

		free(messages);

		return 0;
	}
	else
	{
		printf("%s: no codecs available!\n", __FUNCTION__);
		return -1;
	}

	return 0;
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
