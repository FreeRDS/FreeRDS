/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * Bitmap Encoder
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

#ifndef FREERDS_CORE_ENCODER_H
#define FREERDS_CORE_ENCODER_H

#include <freerdp/freerdp.h>
#include <freerdp/codec/rfx.h>
#include <freerdp/codec/nsc.h>
#include <freerdp/codec/bitmap.h>

#include <winpr/crt.h>
#include <winpr/stream.h>

struct rds_bitmap_encoder
{
	int maxWidth;
	int maxHeight;

	int desktopWidth;
	int desktopHeight;

	UINT32 bitsPerPixel;
	UINT32 bytesPerPixel;

	wStream* bs;
	wStream* bts;

	BITMAP_PLANAR_CONTEXT* planar_context;

	wStream* rfx_s;
	RFX_CONTEXT* rfx_context;

	wStream* nsc_s;
	NSC_CONTEXT* nsc_context;

	BYTE** grid;
	int gridWidth;
	int gridHeight;
	BYTE* gridBuffer;
};
typedef struct rds_bitmap_encoder rdsBitmapEncoder;

rdsBitmapEncoder* freerds_bitmap_encoder_new(int desktopWidth, int desktopHeight, int colorDepth);
void freerds_bitmap_encoder_free(rdsBitmapEncoder* encoder);

#endif /* FREERDS_CORE_ENCODER_H */
