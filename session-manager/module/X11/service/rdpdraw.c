/*
Copyright 2005-2012 Jay Sorg

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Xserver drawing ops and funcs

 */

#include "rdp.h"
#include "gcops.h"
#include "rdpdraw.h"

#include "rdpCopyArea.h"
#include "rdpPolyFillRect.h"
#include "rdpPutImage.h"
#include "rdpPolyRectangle.h"
#include "rdpPolylines.h"
#include "rdpPolySegment.h"
#include "rdpFillSpans.h"
#include "rdpSetSpans.h"
#include "rdpCopyPlane.h"
#include "rdpPolyPoint.h"
#include "rdpPolyArc.h"
#include "rdpFillPolygon.h"
#include "rdpPolyFillArc.h"
#include "rdpPolyText8.h"
#include "rdpPolyText16.h"
#include "rdpImageText8.h"
#include "rdpImageText16.h"
#include "rdpImageGlyphBlt.h"
#include "rdpPolyGlyphBlt.h"
#include "rdpPushPixels.h"

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
		do { if (_level < LOG_LEVEL) { ErrorF _args ; } } while (0)
#define LLOGLN(_level, _args) \
		do { if (_level < LOG_LEVEL) { ErrorF _args ; ErrorF("\n"); } } while (0)

extern rdpScreenInfoRec g_rdpScreen;
extern DevPrivateKeyRec g_rdpGCIndex;
extern DevPrivateKeyRec g_rdpWindowIndex;
extern DevPrivateKeyRec g_rdpPixmapIndex;
extern ScreenPtr g_pScreen;
extern Bool g_wrapPixmap;
extern WindowPtr g_invalidate_window;
extern int g_use_rail;
extern int g_con_number;

ColormapPtr g_rdpInstalledColormap;

static int g_doing_font = 0;

GCFuncs g_rdpGCFuncs =
{
	rdpValidateGC,
	rdpChangeGC,
	rdpCopyGC,
	rdpDestroyGC,
	rdpChangeClip,
	rdpDestroyClip,
	rdpCopyClip
};

GCOps g_rdpGCOps =
{
	rdpFillSpans,
	rdpSetSpans,
	rdpPutImage,
	rdpCopyArea,
	rdpCopyPlane,
	rdpPolyPoint,
	rdpPolylines,
	rdpPolySegment,
	rdpPolyRectangle,
	rdpPolyArc,
	rdpFillPolygon,
	rdpPolyFillRect,
	rdpPolyFillArc,
	rdpPolyText8,
	rdpPolyText16,
	rdpImageText8,
	rdpImageText16,
	rdpImageGlyphBlt,
	rdpPolyGlyphBlt,
	rdpPushPixels
};

/* return 0, draw nothing */
/* return 1, draw with no clip */
/* return 2, draw using clip */
int rdp_get_clip(RegionPtr pRegion, DrawablePtr pDrawable, GCPtr pGC)
{
	int status;
	BoxRec box;
	RegionPtr temp;
	WindowPtr pWindow;

	status = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		switch (pGC->clientClipType)
		{
			case CT_NONE:
				status = 1;
				break;

			case CT_REGION:
				status = 2;
				RegionCopy(pRegion, pGC->clientClip);
				break;

			default:
				rdpLog("unimp clip type %d\n", pGC->clientClipType);
				break;
		}

		if (status == 2) /* check if the clip is the entire pixmap */
		{
			box.x1 = 0;
			box.y1 = 0;
			box.x2 = pDrawable->width;
			box.y2 = pDrawable->height;

			if (RegionContainsRect(pRegion, &box) == rgnIN)
			{
				status = 1;
			}
		}
	}
	else if (pDrawable->type == DRAWABLE_WINDOW)
	{
		pWindow = (WindowPtr) pDrawable;

		if (pWindow->viewable)
		{
			if (pGC->subWindowMode == IncludeInferiors)
			{
				temp = &pWindow->borderClip;
			}
			else
			{
				temp = &pWindow->clipList;
			}

			if (RegionNotEmpty(temp))
			{
				switch (pGC->clientClipType)
				{
					case CT_NONE:
						status = 2;
						RegionCopy(pRegion, temp);
						break;

					case CT_REGION:
						status = 2;
						RegionCopy(pRegion, pGC->clientClip);
						RegionTranslate(pRegion,
								pDrawable->x + pGC->clipOrg.x,
								pDrawable->y + pGC->clipOrg.y);
						RegionIntersect(pRegion, pRegion, temp);
						break;

					default:
						rdpLog("unimp clip type %d\n", pGC->clientClipType);
						break;
				}

				if (status == 2) /* check if the clip is the entire screen */
				{
					box.x1 = 0;
					box.y1 = 0;
					box.x2 = g_rdpScreen.width;
					box.y2 = g_rdpScreen.height;

					if (RegionContainsRect(pRegion, &box) == rgnIN)
					{
						status = 1;
					}
				}
			}
		}
	}

	return status;
}

void GetTextBoundingBox(DrawablePtr pDrawable, FontPtr font, int x, int y, int n, BoxPtr pbox)
{
	int maxAscent;
	int maxDescent;
	int maxCharWidth;

	if (FONTASCENT(font) > FONTMAXBOUNDS(font, ascent))
	{
		maxAscent = FONTASCENT(font);
	}
	else
	{
		maxAscent = FONTMAXBOUNDS(font, ascent);
	}

	if (FONTDESCENT(font) > FONTMAXBOUNDS(font, descent))
	{
		maxDescent = FONTDESCENT(font);
	}
	else
	{
		maxDescent = FONTMAXBOUNDS(font, descent);
	}

	if (FONTMAXBOUNDS(font, rightSideBearing) > FONTMAXBOUNDS(font, characterWidth))
	{
		maxCharWidth = FONTMAXBOUNDS(font, rightSideBearing);
	}
	else
	{
		maxCharWidth = FONTMAXBOUNDS(font, characterWidth);
	}

	pbox->x1 = pDrawable->x + x;
	pbox->y1 = pDrawable->y + y - maxAscent;
	pbox->x2 = pbox->x1 + maxCharWidth * n;
	pbox->y2 = pbox->y1 + maxAscent + maxDescent;

	if (FONTMINBOUNDS(font, leftSideBearing) < 0)
	{
		pbox->x1 += FONTMINBOUNDS(font, leftSideBearing);
	}
}

#define GC_FUNC_PROLOGUE(_pGC) \
		{ \
			priv = (rdpGCPtr)(dixGetPrivateAddr(&(_pGC->devPrivates), &g_rdpGCIndex)); \
			(_pGC)->funcs = priv->funcs; \
			if (priv->ops != 0) \
			{ \
				(_pGC)->ops = priv->ops; \
			} \
		}

#define GC_FUNC_EPILOGUE(_pGC) \
		{ \
	priv->funcs = (_pGC)->funcs; \
	(_pGC)->funcs = &g_rdpGCFuncs; \
	if (priv->ops != 0) \
	{ \
		priv->ops = (_pGC)->ops; \
		(_pGC)->ops = &g_rdpGCOps; \
	} \
		}

static void rdpValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr d)
{
	rdpGCRec *priv;
	int wrap;
	RegionPtr pRegion;

	LLOGLN(10, ("rdpValidateGC:"));
	GC_FUNC_PROLOGUE(pGC);
	pGC->funcs->ValidateGC(pGC, changes, d);

	if (g_wrapPixmap)
	{
		wrap = 1;
	}
	else
	{
		wrap = (d->type == DRAWABLE_WINDOW) && ((WindowPtr)d)->viewable;

		if (wrap)
		{
			if (pGC->subWindowMode == IncludeInferiors)
			{
				pRegion = &(((WindowPtr)d)->borderClip);
			}
			else
			{
				pRegion = &(((WindowPtr)d)->clipList);
			}

			wrap = RegionNotEmpty(pRegion);
		}
	}

	priv->ops = 0;

	if (wrap)
	{
		priv->ops = pGC->ops;
	}

	GC_FUNC_EPILOGUE(pGC);
}

static void rdpChangeGC(GCPtr pGC, unsigned long mask)
{
	rdpGCRec *priv;

	LLOGLN(10, ("in rdpChangeGC"));
	GC_FUNC_PROLOGUE(pGC);
	pGC->funcs->ChangeGC(pGC, mask);
	GC_FUNC_EPILOGUE(pGC);
}

static void rdpCopyGC(GCPtr src, unsigned long mask, GCPtr dst)
{
	rdpGCRec *priv;

	LLOGLN(10, ("in rdpCopyGC"));
	GC_FUNC_PROLOGUE(dst);
	dst->funcs->CopyGC(src, mask, dst);
	GC_FUNC_EPILOGUE(dst);
}

static void rdpDestroyGC(GCPtr pGC)
{
	rdpGCRec *priv;

	LLOGLN(10, ("in rdpDestroyGC"));
	GC_FUNC_PROLOGUE(pGC);
	pGC->funcs->DestroyGC(pGC);
	GC_FUNC_EPILOGUE(pGC);
}

static void rdpChangeClip(GCPtr pGC, int type, pointer pValue, int nrects)
{
	rdpGCRec *priv;

	LLOGLN(10, ("in rdpChangeClip"));
	GC_FUNC_PROLOGUE(pGC);
	pGC->funcs->ChangeClip(pGC, type, pValue, nrects);
	GC_FUNC_EPILOGUE(pGC);
}

static void rdpDestroyClip(GCPtr pGC)
{
	rdpGCRec *priv;

	LLOGLN(10, ("in rdpDestroyClip"));
	GC_FUNC_PROLOGUE(pGC);
	pGC->funcs->DestroyClip(pGC);
	GC_FUNC_EPILOGUE(pGC);
}

static void rdpCopyClip(GCPtr dst, GCPtr src)
{
	rdpGCRec *priv;

	LLOGLN(0, ("in rdpCopyClip"));
	GC_FUNC_PROLOGUE(dst);
	dst->funcs->CopyClip(dst, src);
	GC_FUNC_EPILOGUE(dst);
}

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

Bool rdpCloseScreen(int i, ScreenPtr pScreen)
{
	LLOGLN(10, ("in rdpCloseScreen"));
	pScreen->CloseScreen = g_rdpScreen.CloseScreen;
	pScreen->CreateGC = g_rdpScreen.CreateGC;
	//pScreen->PaintWindowBackground = g_rdpScreen.PaintWindowBackground;
	//pScreen->PaintWindowBorder = g_rdpScreen.PaintWindowBorder;
	pScreen->CopyWindow = g_rdpScreen.CopyWindow;
	pScreen->ClearToBackground = g_rdpScreen.ClearToBackground;
	//pScreen->RestoreAreas = g_rdpScreen.RestoreAreas;
	return 1;
}

PixmapPtr rdpCreatePixmap(ScreenPtr pScreen, int width, int height, int depth, unsigned usage_hint)
{
	PixmapPtr rv;
	rdpPixmapRec* priv;
	int org_width;

	org_width = width;

	/* width must be a multiple of 4 in rdp */
	width = (width + 3) & ~3;

	LLOGLN(10, ("rdpCreatePixmap: width %d org_width %d height %d depth %d screen depth %d usage_hint %d",
			width, org_width, height, depth, g_rdpScreen.depth, usage_hint));

	pScreen->CreatePixmap = g_rdpScreen.CreatePixmap;
	rv = pScreen->CreatePixmap(pScreen, width, height, depth, usage_hint);
	priv = GETPIXPRIV(rv);
	priv->con_number = g_con_number;
	priv->kind_width = width;
	pScreen->ModifyPixmapHeader(rv, org_width, height, depth, 0, 0, 0);
	pScreen->CreatePixmap = rdpCreatePixmap;

	return rv;
}

Bool rdpDestroyPixmap(PixmapPtr pPixmap)
{
	Bool status;
	ScreenPtr pScreen;
	rdpPixmapRec* priv;

	LLOGLN(10, ("rdpDestroyPixmap:"));
	priv = GETPIXPRIV(pPixmap);
	LLOGLN(10, ("status %d refcnt %d", priv->status, pPixmap->refcnt));

	pScreen = pPixmap->drawable.pScreen;
	pScreen->DestroyPixmap = g_rdpScreen.DestroyPixmap;
	status = pScreen->DestroyPixmap(pPixmap);
	pScreen->DestroyPixmap = rdpDestroyPixmap;

	return status;
}

Bool rdpCreateWindow(WindowPtr pWindow)
{
	Bool rv;
	ScreenPtr pScreen;
	rdpWindowRec *priv;

	LLOGLN(10, ("rdpCreateWindow:"));
	priv = GETWINPRIV(pWindow);
	LLOGLN(10, ("  %p status %d", priv, priv->status));

	pScreen = pWindow->drawable.pScreen;
	pScreen->CreateWindow = g_rdpScreen.CreateWindow;
	rv = pScreen->CreateWindow(pWindow);
	pScreen->CreateWindow = rdpCreateWindow;

	if (g_use_rail)
	{
	}

	return rv;
}

Bool rdpDestroyWindow(WindowPtr pWindow)
{
	Bool rv;
	ScreenPtr pScreen;
	//rdpWindowRec *priv;

	LLOGLN(10, ("rdpDestroyWindow:"));
	//priv = GETWINPRIV(pWindow);

	pScreen = pWindow->drawable.pScreen;
	pScreen->DestroyWindow = g_rdpScreen.DestroyWindow;
	rv = pScreen->DestroyWindow(pWindow);
	pScreen->DestroyWindow = rdpDestroyWindow;

	if (g_use_rail)
	{

	}

	return rv;
}

Bool rdpPositionWindow(WindowPtr pWindow, int x, int y)
{
	Bool rv;
	ScreenPtr pScreen;
	rdpWindowRec *priv;

	LLOGLN(10, ("rdpPositionWindow:"));
	priv = GETWINPRIV(pWindow);
	pScreen = pWindow->drawable.pScreen;
	pScreen->PositionWindow = g_rdpScreen.PositionWindow;
	rv = pScreen->PositionWindow(pWindow, x, y);
	pScreen->PositionWindow = rdpPositionWindow;

	if (g_use_rail)
	{
		if (priv->status == 1)
		{
			LLOGLN(10, ("rdpPositionWindow:"));
			LLOGLN(10, ("  x %d y %d", x, y));
		}
	}

	return rv;
}

Bool rdpRealizeWindow(WindowPtr pWindow)
{
	Bool rv;
	ScreenPtr pScreen;
	rdpWindowRec *priv;

	LLOGLN(10, ("rdpRealizeWindow:"));
	priv = GETWINPRIV(pWindow);
	pScreen = pWindow->drawable.pScreen;
	pScreen->RealizeWindow = g_rdpScreen.RealizeWindow;
	rv = pScreen->RealizeWindow(pWindow);
	pScreen->RealizeWindow = rdpRealizeWindow;

	if (g_use_rail)
	{
		if ((pWindow != g_invalidate_window) && (pWindow->parent != 0))
		{
			if (XR_IS_ROOT(pWindow->parent))
			{
				LLOGLN(10, ("rdpRealizeWindow:"));
				LLOGLN(10, ("  pWindow %p id 0x%x pWindow->parent %p id 0x%x x %d "
						"y %d width %d height %d",
						pWindow, (int)(pWindow->drawable.id),
						pWindow->parent, (int)(pWindow->parent->drawable.id),
						pWindow->drawable.x, pWindow->drawable.y,
						pWindow->drawable.width, pWindow->drawable.height));
				priv->status = 1;
				rdpup_create_window(pWindow, priv);
			}
		}
	}

	return rv;
}

Bool rdpUnrealizeWindow(WindowPtr pWindow)
{
	Bool rv;
	ScreenPtr pScreen;
	rdpWindowRec *priv;

	LLOGLN(10, ("rdpUnrealizeWindow:"));
	priv = GETWINPRIV(pWindow);
	pScreen = pWindow->drawable.pScreen;
	pScreen->UnrealizeWindow = g_rdpScreen.UnrealizeWindow;
	rv = pScreen->UnrealizeWindow(pWindow);
	pScreen->UnrealizeWindow = rdpUnrealizeWindow;

	if (g_use_rail)
	{
		if (priv->status == 1)
		{
			LLOGLN(10, ("rdpUnrealizeWindow:"));
			priv->status = 0;
			rdpup_delete_window(pWindow, priv);
		}
	}

	return rv;
}

Bool rdpChangeWindowAttributes(WindowPtr pWindow, unsigned long mask)
{
	Bool rv;
	ScreenPtr pScreen;
	//rdpWindowRec *priv;

	LLOGLN(10, ("rdpChangeWindowAttributes:"));
	//priv = GETWINPRIV(pWindow);
	pScreen = pWindow->drawable.pScreen;
	pScreen->ChangeWindowAttributes = g_rdpScreen.ChangeWindowAttributes;
	rv = pScreen->ChangeWindowAttributes(pWindow, mask);
	pScreen->ChangeWindowAttributes = rdpChangeWindowAttributes;

	if (g_use_rail)
	{

	}

	return rv;
}

void rdpWindowExposures(WindowPtr pWindow, RegionPtr pRegion, RegionPtr pBSRegion)
{
	ScreenPtr pScreen;
	//rdpWindowRec *priv;

	LLOGLN(10, ("rdpWindowExposures:"));
	//priv = GETWINPRIV(pWindow);
	pScreen = pWindow->drawable.pScreen;
	pScreen->WindowExposures = g_rdpScreen.WindowExposures;
	pScreen->WindowExposures(pWindow, pRegion, pBSRegion);

	if (g_use_rail)
	{

	}

	pScreen->WindowExposures = rdpWindowExposures;
}

Bool rdpCreateGC(GCPtr pGC)
{
	rdpGCRec *priv;
	Bool rv;

	LLOGLN(10, ("in rdpCreateGC\n"));
	priv = GETGCPRIV(pGC);
	g_pScreen->CreateGC = g_rdpScreen.CreateGC;
	rv = g_pScreen->CreateGC(pGC);

	if (rv)
	{
		priv->funcs = pGC->funcs;
		priv->ops = 0;
		pGC->funcs = &g_rdpGCFuncs;
	}
	else
	{
		rdpLog("error in rdpCreateGC, CreateGC failed\n");
	}

	g_pScreen->CreateGC = rdpCreateGC;
	return rv;
}

void rdpCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr pOldRegion)
{
	RegionRec reg;
	RegionRec clip;
	int dx;
	int dy;
	int i;
	int j;
	int num_clip_rects;
	int num_reg_rects;
	BoxRec box1;
	BoxRec box2;

	LLOGLN(10, ("in rdpCopyWindow"));
	RegionInit(&reg, NullBox, 0);
	RegionCopy(&reg, pOldRegion);
	g_pScreen->CopyWindow = g_rdpScreen.CopyWindow;
	g_pScreen->CopyWindow(pWin, ptOldOrg, pOldRegion);
	RegionInit(&clip, NullBox, 0);
	RegionCopy(&clip, &pWin->borderClip);
	dx = pWin->drawable.x - ptOldOrg.x;
	dy = pWin->drawable.y - ptOldOrg.y;

	rdpup_begin_update();
	num_clip_rects = REGION_NUM_RECTS(&clip);
	num_reg_rects = REGION_NUM_RECTS(&reg);

	/* should maybe sort the rects instead of checking dy < 0 */
	/* If we can depend on the rects going from top to bottom, left to right we are ok */
	if (dy < 0 || (dy == 0 && dx < 0))
	{
		for (j = 0; j < num_clip_rects; j++)
		{
			box1 = REGION_RECTS(&clip)[j];
			rdpup_set_clip(box1.x1, box1.y1, box1.x2 - box1.x1, box1.y2 - box1.y1);

			for (i = 0; i < num_reg_rects; i++)
			{
				box2 = REGION_RECTS(&reg)[i];
				rdpup_screen_blt(box2.x1 + dx, box2.y1 + dy, box2.x2 - box2.x1,
						box2.y2 - box2.y1, box2.x1, box2.y1);
			}
		}
	}
	else
	{
		for (j = num_clip_rects - 1; j >= 0; j--)
		{
			box1 = REGION_RECTS(&clip)[j];
			rdpup_set_clip(box1.x1, box1.y1, box1.x2 - box1.x1, box1.y2 - box1.y1);

			for (i = num_reg_rects - 1; i >= 0; i--)
			{
				box2 = REGION_RECTS(&reg)[i];
				rdpup_screen_blt(box2.x1 + dx, box2.y1 + dy, box2.x2 - box2.x1,
						box2.y2 - box2.y1, box2.x1, box2.y1);
			}
		}
	}

	rdpup_reset_clip();
	rdpup_end_update();

	RegionUninit(&reg);
	RegionUninit(&clip);
	g_pScreen->CopyWindow = rdpCopyWindow;
}

void rdpClearToBackground(WindowPtr pWin, int x, int y, int w, int h, Bool generateExposures)
{
	int j;
	BoxRec box;
	RegionRec reg;

	LLOGLN(10, ("in rdpClearToBackground"));
	g_pScreen->ClearToBackground = g_rdpScreen.ClearToBackground;
	g_pScreen->ClearToBackground(pWin, x, y, w, h, generateExposures);

	if (!generateExposures)
	{
		if (w > 0 && h > 0)
		{
			box.x1 = x;
			box.y1 = y;
			box.x2 = box.x1 + w;
			box.y2 = box.y1 + h;
		}
		else
		{
			box.x1 = pWin->drawable.x;
			box.y1 = pWin->drawable.y;
			box.x2 = box.x1 + pWin->drawable.width;
			box.y2 = box.y1 + pWin->drawable.height;
		}

		RegionInit(&reg, &box, 0);
		RegionIntersect(&reg, &reg, &pWin->clipList);

		rdpup_begin_update();

		for (j = REGION_NUM_RECTS(&reg) - 1; j >= 0; j--)
		{
			box = REGION_RECTS(&reg)[j];
			rdpup_send_area(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
		}

		rdpup_end_update();

		RegionUninit(&reg);
	}

	g_pScreen->ClearToBackground = rdpClearToBackground;
}

RegionPtr rdpRestoreAreas(WindowPtr pWin, RegionPtr prgnExposed)
{
	int j;
	BoxRec box;
	RegionRec reg;
	RegionPtr rv = NULL;

	LLOGLN(0, ("in rdpRestoreAreas"));
	RegionInit(&reg, NullBox, 0);
	RegionCopy(&reg, prgnExposed);

	rdpup_begin_update();

	for (j = REGION_NUM_RECTS(&reg) - 1; j >= 0; j--)
	{
		box = REGION_RECTS(&reg)[j];
		rdpup_send_area(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
	}

	rdpup_end_update();

	RegionUninit(&reg);

	return rv;
}

void rdpInstallColormap(ColormapPtr pmap)
{
	ColormapPtr oldpmap;

	oldpmap = g_rdpInstalledColormap;

	if (pmap != oldpmap)
	{
		if (oldpmap != (ColormapPtr)None)
		{
			WalkTree(pmap->pScreen, TellLostMap, (char *)&oldpmap->mid);
		}

		/* Install pmap */
		g_rdpInstalledColormap = pmap;
		WalkTree(pmap->pScreen, TellGainedMap, (char *)&pmap->mid);
		/*rfbSetClientColourMaps(0, 0);*/
	}

	/*g_rdpScreen.InstallColormap(pmap);*/
}

void rdpUninstallColormap(ColormapPtr pmap)
{
	ColormapPtr curpmap;

	curpmap = g_rdpInstalledColormap;

	if (pmap == curpmap)
	{
		if (pmap->mid != pmap->pScreen->defColormap)
		{
			//curpmap = (ColormapPtr)LookupIDByType(pmap->pScreen->defColormap,
			//                                      RT_COLORMAP);
			//pmap->pScreen->InstallColormap(curpmap);
		}
	}
}

int rdpListInstalledColormaps(ScreenPtr pScreen, Colormap *pmaps)
{
	*pmaps = g_rdpInstalledColormap->mid;
	return 1;
}

void rdpStoreColors(ColormapPtr pmap, int ndef, xColorItem *pdefs)
{
}

Bool rdpSaveScreen(ScreenPtr pScreen, int on)
{
	return 1;
}

/* it looks like all the antialias draws go through here */
void rdpComposite(CARD8 op, PicturePtr pSrc, PicturePtr pMask, PicturePtr pDst,
		INT16 xSrc, INT16 ySrc, INT16 xMask, INT16 yMask, INT16 xDst,
		INT16 yDst, CARD16 width, CARD16 height)
{
	BoxRec box;
	PictureScreenPtr ps;
	RegionRec reg1;
	RegionRec reg2;
	DrawablePtr p;
	int j;
	int num_clips;
	int post_process;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	LLOGLN(10, ("rdpComposite:"));

	ps = GetPictureScreen(g_pScreen);
	ps->Composite = g_rdpScreen.Composite;
	ps->Composite(op, pSrc, pMask, pDst, xSrc, ySrc, xMask, yMask, xDst, yDst, width, height);
	ps->Composite = rdpComposite;

	p = pDst->pDrawable;

	post_process = 0;

	if (p->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) p;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (p->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) p;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
		return;

	if (pDst->pCompositeClip != 0)
	{
		box.x1 = p->x + xDst;
		box.y1 = p->y + yDst;
		box.x2 = box.x1 + width;
		box.y2 = box.y1 + height;
		RegionInit(&reg1, &box, 0);
		RegionInit(&reg2, NullBox, 0);
		RegionCopy(&reg2, pDst->pCompositeClip);
		RegionIntersect(&reg1, &reg1, &reg2);

		num_clips = REGION_NUM_RECTS(&reg1);

		if (num_clips > 0)
		{
			rdpup_begin_update();

			for (j = num_clips - 1; j >= 0; j--)
			{
				box = REGION_RECTS(&reg1)[j];
				rdpup_send_area(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
			}

			rdpup_end_update();
		}

		RegionUninit(&reg1);
		RegionUninit(&reg2);
	}
	else
	{
		box.x1 = p->x + xDst;
		box.y1 = p->y + yDst;
		box.x2 = box.x1 + width;
		box.y2 = box.y1 + height;

		rdpup_begin_update();
		rdpup_send_area(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
		rdpup_end_update();
	}
}

void GlyphExtents(int nlist, GlyphListPtr list, GlyphPtr* glyphs, BoxPtr extents)
{
	int x1;
	int x2;
	int y1;
	int y2;
	int n;
	int x;
	int y;
	GlyphPtr glyph;

	x = 0;
	y = 0;
	extents->x1 = MAXSHORT;
	extents->x2 = MINSHORT;
	extents->y1 = MAXSHORT;
	extents->y2 = MINSHORT;

	while (nlist--)
	{
		x += list->xOff;
		y += list->yOff;
		n = list->len;
		list++;

		while (n--)
		{
			glyph = *glyphs++;
			x1 = x - glyph->info.x;
			if (x1 < MINSHORT)
			{
				x1 = MINSHORT;
			}
			y1 = y - glyph->info.y;
			if (y1 < MINSHORT)
			{
				y1 = MINSHORT;
			}
			x2 = x1 + glyph->info.width;
			if (x2 > MAXSHORT)
			{
				x2 = MAXSHORT;
			}
			y2 = y1 + glyph->info.height;
			if (y2 > MAXSHORT)
			{
				y2 = MAXSHORT;
			}
			if (x1 < extents->x1)
			{
				extents->x1 = x1;
			}
			if (x2 > extents->x2)
			{
				extents->x2 = x2;
			}
			if (y1 < extents->y1)
			{
				extents->y1 = y1;
			}
			if (y2 > extents->y2)
			{
				extents->y2 = y2;
			}
			x += glyph->info.xOff;
			y += glyph->info.yOff;
		}
	}
}

void rdpGlyphs(CARD8 op, PicturePtr pSrc, PicturePtr pDst, PictFormatPtr maskFormat,
		INT16 xSrc, INT16 ySrc, int nlists, GlyphListPtr lists, GlyphPtr *glyphs)
{
	int index;
	BoxRec box;
	PictureScreenPtr ps;

	LLOGLN(10, ("rdpGlyphs:"));
	LLOGLN(10, ("rdpGlyphs: nlists %d len %d", nlists, lists->len));

	g_doing_font = 1;

	for (index = 0; index < lists->len; index++)
	{
		LLOGLN(10, ("  index %d size %d refcnt %d width %d height %d",
				index, (int)(glyphs[index]->size), (int)(glyphs[index]->refcnt),
				glyphs[index]->info.width, glyphs[index]->info.height));
	}

	ps = GetPictureScreen(g_pScreen);
	ps->Glyphs = g_rdpScreen.Glyphs;
	ps->Glyphs(op, pSrc, pDst, maskFormat, xSrc, ySrc, nlists, lists, glyphs);
	ps->Glyphs = rdpGlyphs;

	g_doing_font = 0;

	GlyphExtents(nlists, lists, glyphs, &box);

	rdpup_begin_update();
	rdpup_send_area(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
	rdpup_end_update();

	LLOGLN(10, ("rdpGlyphs: out"));
}
