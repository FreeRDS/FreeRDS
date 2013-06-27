/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * xrdp-ng interprocess communication protocol
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

#include <xrdp-ng/xrdp.h>

int xrdp_write_header(wStream* s, int type, int length)
{
	Stream_Write_UINT16(s, type);
	Stream_Write_UINT32(s, length);

	return 0;
}

int xrdp_write_begin_update(wStream* s)
{
	xrdp_write_header(s, XRDP_SERVER_BEGIN_UPDATE, 6);
	return 0;
}

int xrdp_write_end_update(wStream* s)
{
	xrdp_write_header(s, XRDP_SERVER_END_UPDATE, 6);
	return 0;
}

int xrdp_write_fill_rect(wStream* s, int x, int y, int width, int height)
{
	xrdp_write_header(s, XRDP_SERVER_FILL_RECT, 14);
	Stream_Write_UINT16(s, x);
	Stream_Write_UINT16(s, y);
	Stream_Write_UINT16(s, width);
	Stream_Write_UINT16(s, height);
	return 0;
}

int xrdp_write_screen_blt(wStream* s, int x, int y, int width, int height, int srcx, int srcy)
{
	xrdp_write_header(s, XRDP_SERVER_SCREEN_BLT, 18);
	Stream_Write_UINT16(s, x);
	Stream_Write_UINT16(s, y);
	Stream_Write_UINT16(s, width);
	Stream_Write_UINT16(s, height);
	Stream_Write_UINT16(s, srcx);
	Stream_Write_UINT16(s, srcy);
	return 0;
}

int xrdp_write_set_clip(wStream* s, int x, int y, int width, int height)
{
	xrdp_write_header(s, XRDP_SERVER_SET_CLIP, 14);
	Stream_Write_UINT16(s, x);
	Stream_Write_UINT16(s, y);
	Stream_Write_UINT16(s, width);
	Stream_Write_UINT16(s, height);
	return 0;
}

int xrdp_write_reset_clip(wStream* s)
{
	xrdp_write_header(s, XRDP_SERVER_RESET_CLIP, 6);
	return 0;
}

int xrdp_write_set_forecolor(wStream* s, UINT32 color)
{
	xrdp_write_header(s, XRDP_SERVER_SET_FORECOLOR, 10);
	Stream_Write_UINT32(s, color);
	return 0;
}

int xrdp_write_set_backcolor(wStream* s, UINT32 color)
{
	xrdp_write_header(s, XRDP_SERVER_SET_BACKCOLOR, 10);
	Stream_Write_UINT32(s, color);
	return 0;
}

int xrdp_write_set_opcode(wStream* s, UINT32 opcode)
{
	xrdp_write_header(s, XRDP_SERVER_SET_OPCODE, 8);
	Stream_Write_UINT16(s, opcode);
	return 0;
}

int xrdp_write_set_pen(wStream* s, int style, int width)
{
	xrdp_write_header(s, XRDP_SERVER_SET_PEN, 10);
	Stream_Write_UINT16(s, style);
	Stream_Write_UINT16(s, width);
	return 0;
}

int xrdp_write_draw_line(wStream* s, int x1, int y1, int x2, int y2)
{
	xrdp_write_header(s, XRDP_SERVER_DRAW_LINE, 14);
	Stream_Write_UINT16(s, x1);
	Stream_Write_UINT16(s, y1);
	Stream_Write_UINT16(s, x2);
	Stream_Write_UINT16(s, y2);
	return 0;
}

int xrdp_write_set_cursor(wStream* s, int x, int y, char* cur_data, char* cur_mask)
{
	int size = 8 + 32 * (32 * 3) + 32 * (32 / 8) + 2;

	xrdp_write_header(s, XRDP_SERVER_SET_POINTER, size);
	Stream_Write_UINT16(s, x);
	Stream_Write_UINT16(s, y);
	Stream_Write(s, cur_data, 32 * (32 * 3));
	Stream_Write(s, cur_mask, 32 * (32 / 8));
	return 0;
}

int xrdp_write_set_cursor_ex(wStream* s, int x, int y, char* cur_data, char* cur_mask, int bpp)
{
	int BytesPerPixel = (bpp == 0) ? 3 : (bpp + 7) / 8;
	int size = 10 + 32 * (32 * BytesPerPixel) + 32 * (32 / 8) + 2;

	xrdp_write_header(s, XRDP_SERVER_SET_POINTER_EX, size);
	Stream_Write_UINT16(s, x);
	Stream_Write_UINT16(s, y);
	Stream_Write_UINT16(s, bpp);
	Stream_Write(s, cur_data, 32 * (32 * BytesPerPixel));
	Stream_Write(s, cur_mask, 32 * (32 / 8));

	return 0;
}

int xrdp_write_create_os_surface(wStream* s, int index, int width, int height)
{
	xrdp_write_header(s, XRDP_SERVER_CREATE_OS_SURFACE, 14);
	Stream_Write_UINT32(s, index);
	Stream_Write_UINT16(s, width);
	Stream_Write_UINT16(s, height);

	return 0;
}

int xrdp_write_switch_os_surface(wStream* s, int index)
{
	xrdp_write_header(s, XRDP_SERVER_SWITCH_OS_SURFACE, 10);
	Stream_Write_UINT32(s, index);

	return 0;
}

int xrdp_write_delete_os_surface(wStream* s, int index)
{
	xrdp_write_header(s, XRDP_SERVER_DELETE_OS_SURFACE, 10);
	Stream_Write_UINT32(s, index);

	return 0;
}

int xrdp_write_paint_rect_os(wStream* s, int x, int y, int width, int height, int index, int srcx, int srcy)
{
	xrdp_write_header(s, XRDP_SERVER_PAINT_RECT_OS, 22);

	Stream_Write_UINT16(s, x);
	Stream_Write_UINT16(s, y);
	Stream_Write_UINT16(s, width);
	Stream_Write_UINT16(s, height);
	Stream_Write_UINT32(s, index);
	Stream_Write_UINT16(s, srcx);
	Stream_Write_UINT16(s, srcy);

	return 0;
}

int xrdp_write_set_hints(wStream* s, int hints, int mask)
{
	xrdp_write_header(s, XRDP_SERVER_SET_HINTS, 14);

	Stream_Write_UINT32(s, hints);
	Stream_Write_UINT32(s, mask);

	return 0;
}
