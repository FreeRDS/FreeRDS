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

#define GC_OP_PROLOGUE(_pGC) \
{ \
	priv = (rdpGCPtr)dixGetPrivateAddr(&(pGC->devPrivates), &g_rdpGCIndex); \
	oldFuncs = _pGC->funcs; \
	(_pGC)->funcs = priv->funcs; \
	(_pGC)->ops = priv->ops; \
}

#define GC_OP_EPILOGUE(_pGC) \
{ \
	priv->ops = (_pGC)->ops; \
	(_pGC)->funcs = oldFuncs; \
	(_pGC)->ops = &g_rdpGCOps; \
}

int rdp_get_clip(RegionPtr pRegion, DrawablePtr pDrawable, GCPtr pGC);
void GetTextBoundingBox(DrawablePtr pDrawable, FontPtr font, int x, int y, int n, BoxPtr pbox);

Bool rdpCloseScreen(ScreenPtr pScreen);
void rdpQueryBestSize(int xclass, unsigned short* pWidth, unsigned short* pHeight, ScreenPtr pScreen);

PixmapPtr rdpCreatePixmap(ScreenPtr pScreen, int width, int height, int depth, unsigned usage_hint);
Bool rdpDestroyPixmap(PixmapPtr pPixmap);
Bool rdpCreateWindow(WindowPtr pWindow);
Bool rdpDestroyWindow(WindowPtr pWindow);
Bool rdpPositionWindow(WindowPtr pWindow, int x, int y);
Bool rdpRealizeWindow(WindowPtr pWindow);
Bool rdpUnrealizeWindow(WindowPtr pWindow);
Bool rdpChangeWindowAttributes(WindowPtr pWindow, unsigned long mask);
void rdpWindowExposures(WindowPtr pWindow, RegionPtr pRegion, RegionPtr pBSRegion);

Bool rdpCreateGC(GCPtr pGC);
void rdpCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr pOldRegion);
void rdpClearToBackground(WindowPtr pWin, int x, int y, int w, int h, Bool generateExposures);
RegionPtr rdpRestoreAreas(WindowPtr pWin, RegionPtr prgnExposed);
void rdpInstallColormap(ColormapPtr pmap);
void rdpUninstallColormap(ColormapPtr pmap);
int rdpListInstalledColormaps(ScreenPtr pScreen, Colormap* pmaps);
void rdpStoreColors(ColormapPtr pmap, int ndef, xColorItem* pdefs);
Bool rdpSaveScreen(ScreenPtr pScreen, int on);
Bool rdpRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
Bool rdpUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
void rdpCursorLimits(ScreenPtr pScreen, CursorPtr pCursor, BoxPtr pHotBox, BoxPtr pTopLeftBox);
void rdpConstrainCursor(ScreenPtr pScreen, BoxPtr pBox);
Bool rdpSetCursorPosition(ScreenPtr pScreen, int x, int y, Bool generateEvent);
Bool rdpDisplayCursor(ScreenPtr pScreen, CursorPtr pCursor);
void rdpRecolorCursor(ScreenPtr pScreen, CursorPtr pCursor, Bool displayed);
void rdpComposite(CARD8 op, PicturePtr pSrc, PicturePtr pMask, PicturePtr pDst,
		INT16 xSrc, INT16 ySrc, INT16 xMask, INT16 yMask, INT16 xDst,
		INT16 yDst, CARD16 width, CARD16 height);
void rdpGlyphs(CARD8 op, PicturePtr pSrc, PicturePtr pDst, PictFormatPtr maskFormat,
		INT16 xSrc, INT16 ySrc, int nlists, GlyphListPtr lists, GlyphPtr* glyphs);

#endif
