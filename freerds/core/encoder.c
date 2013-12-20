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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "encoder.h"

#include <winpr/crt.h>

rdsBitmapEncoder* freerds_bitmap_encoder_new(int desktopWidth, int desktopHeight, int colorDepth)
{
	DWORD planarFlags;
	rdsBitmapEncoder* encoder;

	encoder = (rdsBitmapEncoder*) malloc(sizeof(rdsBitmapEncoder));

	if (encoder)
	{
		ZeroMemory(encoder, sizeof(rdsBitmapEncoder));

		encoder->maxWidth = 64;
		encoder->maxHeight = 64;

		encoder->desktopWidth = desktopWidth;
		encoder->desktopHeight = desktopHeight;

		encoder->bitsPerPixel = colorDepth;
		encoder->bytesPerPixel = colorDepth / 8;

		encoder->bs = Stream_New(NULL, encoder->maxWidth * encoder->maxHeight * 4);
		encoder->bts = Stream_New(NULL, encoder->maxWidth * encoder->maxHeight * 4);

		planarFlags = PLANAR_FORMAT_HEADER_NA;
		planarFlags |= PLANAR_FORMAT_HEADER_RLE;

		encoder->planar_context = freerdp_bitmap_planar_context_new(planarFlags, encoder->maxWidth, encoder->maxHeight);

		encoder->rfx_context = rfx_context_new(TRUE);
		encoder->rfx_s = Stream_New(NULL, encoder->maxWidth * encoder->maxHeight * 4);

		encoder->rfx_context->mode = RLGR3;
		encoder->rfx_context->width = desktopWidth;
		encoder->rfx_context->height = desktopHeight;

		encoder->nsc_context = nsc_context_new();
		encoder->nsc_s = Stream_New(NULL, encoder->maxWidth * encoder->maxHeight * 4);

		if (encoder->bytesPerPixel == 4)
		{
			rfx_context_set_pixel_format(encoder->rfx_context, RDP_PIXEL_FORMAT_B8G8R8A8);
			nsc_context_set_pixel_format(encoder->nsc_context, RDP_PIXEL_FORMAT_B8G8R8A8);
		}
		else if (encoder->bytesPerPixel == 3)
		{
			rfx_context_set_pixel_format(encoder->rfx_context, RDP_PIXEL_FORMAT_B8G8R8);
			nsc_context_set_pixel_format(encoder->nsc_context, RDP_PIXEL_FORMAT_B8G8R8);
		}
	}

	return encoder;
}

void freerds_bitmap_encoder_free(rdsBitmapEncoder* encoder)
{
	if (encoder)
	{
		Stream_Free(encoder->bs, TRUE);
		Stream_Free(encoder->bts, TRUE);

		Stream_Free(encoder->rfx_s, TRUE);
		rfx_context_free(encoder->rfx_context);

		Stream_Free(encoder->nsc_s, TRUE);
		nsc_context_free(encoder->nsc_context);

		freerdp_bitmap_planar_context_free(encoder->planar_context);

		free(encoder);
	}
}
