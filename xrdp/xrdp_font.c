/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2012
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
 *
 * fonts
 */

/*
  The fv1 files contain
  Font File Header (just one)
    FNT1       4 bytes
    Font Name  32 bytes
    Font Size  2 bytes
    Font Style 2 bytes
    Pad        8 bytes
  Font Data (repeats)
    Width      2 bytes
    Height     2 bytes
    Baseline   2 bytes
    Offset     2 bytes
    Incby      2 bytes
    Pad        6 bytes
    Glyph Data var, see FONT_DATASIZE macro
*/

#include "xrdp.h"
#include "log.h"

xrdpFont* xrdp_font_create(xrdpWm *wm)
{
	xrdpFont *self;
	wStream* s;
	int fd;
	int b;
	int i;
	int index;
	int datasize;
	int file_size;
	xrdpFontChar *f;
	char file_path[256];

	DEBUG(("in xrdp_font_create"));
	g_snprintf(file_path, 255, "%s/%s", XRDP_SHARE_PATH, DEFAULT_FONT_NAME);

	if (!g_file_exist(file_path))
	{
		log_message(LOG_LEVEL_ERROR, "xrdp_font_create: error font file [%s] does not exist", file_path);
		return 0;
	}

	file_size = g_file_get_size(file_path);

	if (file_size < 1)
	{
		log_message(LOG_LEVEL_ERROR, "xrdp_font_create: error reading font from file [%s]", file_path);
		return 0;
	}

	self = (xrdpFont *) g_malloc(sizeof(xrdpFont), 1);
	self->wm = wm;

	s = Stream_New(NULL, file_size + 1024);

	fd = g_file_open(file_path);

	if (fd != -1)
	{
		b = g_file_read(fd, s->buffer, file_size + 1024);
		g_file_close(fd);

		if (b > 0)
		{
			s->length = b;

			Stream_Seek(s, 4);
			Stream_Read(s, self->name, 32);
			Stream_Read_UINT16(s, self->size);
			Stream_Read_UINT16(s, self->style);
			Stream_Seek(s, 8);
			index = 32;

			while (Stream_GetRemainingLength(s) >= 16)
			{
				f = self->font_items + index;
				Stream_Read_INT16(s, i);
				f->width = i;
				Stream_Read_INT16(s, i);
				f->height = i;
				Stream_Read_INT16(s, i);
				f->baseline = i;
				Stream_Read_INT16(s, i);
				f->offset = i;
				Stream_Read_INT16(s, i);
				f->incby = i;
				Stream_Seek(s, 6);
				datasize = FONT_DATASIZE(f);

				if (datasize < 0 || datasize > 512)
				{
					/* shouldn't happen */
					log_message(LOG_LEVEL_ERROR, "error in xrdp_font_create, datasize wrong");
					log_message(LOG_LEVEL_DEBUG, "width %d height %d datasize %d index %d",
							f->width, f->height, datasize, index);
					break;
				}

				if (Stream_GetRemainingLength(s) >= datasize)
				{
					f->data = (char *) g_malloc(datasize, 0);
					Stream_Read(s, f->data, datasize);
				}
				else
				{
					log_message(LOG_LEVEL_ERROR, "error in xrdp_font_create");
				}

				index++;
			}
		}
	}

	Stream_Free(s, TRUE);

	DEBUG(("out xrdp_font_create"));

	return self;
}

/* free the font and all the items */
void xrdp_font_delete(xrdpFont *self)
{
	int i;

	if (!self)
		return;

	for (i = 0; i < NUM_FONTS; i++)
	{
		free(self->font_items[i].data);
	}

	free(self);
}

/* compare the two font items returns 1 if they match */
int xrdp_font_item_compare(xrdpFontChar *font1, xrdpFontChar *font2)
{
	int datasize;

	if (!font1)
		return 0;

	if (!font2)
		return 0;

	if (font1->offset != font2->offset)
		return 0;

	if (font1->baseline != font2->baseline)
		return 0;

	if (font1->width != font2->width)
		return 0;

	if (font1->height != font2->height)
		return 0;

	datasize = FONT_DATASIZE(font1);

	if (g_memcmp(font1->data, font2->data, datasize) == 0)
		return 1;

	return 0;
}
