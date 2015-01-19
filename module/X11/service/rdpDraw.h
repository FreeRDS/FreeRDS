/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2005-2012 Jay Sorg
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

#ifndef __RDPDRAW_H
#define __RDPDRAW_H

int rdp_get_clip(RegionPtr pRegion, DrawablePtr pDrawable, GCPtr pGC);
void GetTextBoundingBox(DrawablePtr pDrawable, FontPtr font, int x, int y, int n, BoxPtr pbox);

PixmapPtr rdpCreatePixmap(ScreenPtr pScreen, int width, int height, int depth, unsigned usage_hint);
Bool rdpDestroyPixmap(PixmapPtr pPixmap);

#include "rdpPalette.h" /* Colormap procedures */
#include "rdpWindow.h" /* Window Procedures */

Bool rdpCreateGC(GCPtr pGC);
void rdpCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr pOldRegion);
void rdpClearToBackground(WindowPtr pWin, int x, int y, int w, int h, Bool generateExposures);
RegionPtr rdpRestoreAreas(WindowPtr pWin, RegionPtr prgnExposed);
void rdpComposite(CARD8 op, PicturePtr pSrc, PicturePtr pMask, PicturePtr pDst,
		INT16 xSrc, INT16 ySrc, INT16 xMask, INT16 yMask, INT16 xDst,
		INT16 yDst, CARD16 width, CARD16 height);
void rdpGlyphs(CARD8 op, PicturePtr pSrc, PicturePtr pDst, PictFormatPtr maskFormat,
		INT16 xSrc, INT16 ySrc, int nlists, GlyphListPtr lists, GlyphPtr* glyphs);

#endif
