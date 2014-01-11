/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2005-2013 Jay Sorg
 * Copyright 2013-2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef FREERDS_X11RDP_UPDATE_H
#define FREERDS_X11RDP_UPDATE_H

int rdp_send_update(RDS_MSG_COMMON* msg);
UINT32 rdp_convert_color(UINT32 color);
UINT32 rdp_convert_opcode(int opcode);
UINT32 rdp_dstblt_rop(int opcode);
int rdp_init(void);
int rdp_check(void);
int rdp_detach_framebuffer();
int rdp_set_clip(int x, int y, int width, int height);
int rdp_reset_clip(void);
void rdp_send_area_update(int x, int y, int width, int height);

#endif /* FREERDS_X11RDP_UPDATE_H */
