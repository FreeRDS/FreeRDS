/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2013-2015 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#include "rdp.h"
#include "rdpGCOps.h"

GCOps g_rdpGCOps;
extern GCFuncs g_rdpGCFuncs;

extern ScreenPtr g_pScreen;
extern rdpScreenInfoRec g_rdpScreen;
extern DevPrivateKeyRec g_rdpGCIndex;
extern DevPrivateKeyRec g_rdpPixmapIndex;

#define GC_OP_PROLOGUE(_pGC) \
{ \
	priv = (rdpGCPtr)dixGetPrivateAddr(&(pGC->devPrivates), &g_rdpGCIndex); \
	oldFuncs = _pGC->funcs; \
	(_pGC)->funcs = (GCFUNCS_TYPE*) priv->funcs; \
	(_pGC)->ops = (GCOPS_TYPE*) priv->ops; \
}

#define GC_OP_EPILOGUE(_pGC) \
{ \
	priv->ops = (_pGC)->ops; \
	(_pGC)->funcs = (GCFUNCS_TYPE*) oldFuncs; \
	(_pGC)->ops = (GCOPS_TYPE*) &g_rdpGCOps; \
}

/**
 * FillSpans
 */

static void rdpFillSpansOrg(DrawablePtr pDrawable, GCPtr pGC, int nInit, DDXPointPtr pptInit, int *pwidthInit, int fSorted)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->FillSpans(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);
	GC_OP_EPILOGUE(pGC);
}

void rdpFillSpans(DrawablePtr pDrawable, GCPtr pGC, int nInit, DDXPointPtr pptInit, int *pwidthInit, int fSorted)
{
	int cd;
	int post_process;
	RegionRec clip_reg;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	/* do original call */
	rdpFillSpansOrg(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
		return;

	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, pDrawable, pGC);

	if (cd == 1)
	{

	}
	else if (cd == 2)
	{

	}
}

/**
 * SetSpans
 */

void rdpSetSpansOrg(DrawablePtr pDrawable, GCPtr pGC, char *psrc,
		DDXPointPtr ppt, int *pwidth, int nspans, int fSorted)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->SetSpans(pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted);
	GC_OP_EPILOGUE(pGC);
}

void rdpSetSpans(DrawablePtr pDrawable, GCPtr pGC, char *psrc,
		DDXPointPtr ppt, int *pwidth, int nspans, int fSorted)
{
	int cd;
	int post_process;
	RegionRec clip_reg;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	/* do original call */
	rdpSetSpansOrg(pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
		return;

	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, pDrawable, pGC);

	if (cd == 1)
	{

	}
	else if (cd == 2)
	{

	}

	RegionUninit(&clip_reg);
}

/**
 * PutImage
 */

static void rdpPutImageOrg(DrawablePtr pDst, GCPtr pGC, int depth, int x, int y, int w, int h, int leftPad, int format, char *pBits)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->PutImage(pDst, pGC, depth, x, y, w, h, leftPad, format, pBits);
	GC_OP_EPILOGUE(pGC);
}

void rdpPutImage(DrawablePtr pDst, GCPtr pGC, int depth, int x, int y, int w, int h, int leftPad, int format, char *pBits)
{
	int j;
	int cd;
	int post_process;
	BoxRec box;
	RegionRec clip_reg;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	/* do original call */
	rdpPutImageOrg(pDst, pGC, depth, x, y, w, h, leftPad, format, pBits);

	post_process = 0;

	if (pDst->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDst;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDst->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDst;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
		return;

	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, pDst, pGC);

	if (cd == 1)
	{
		rdp_send_area_update(pDst->x + x, pDst->y + y, w, h);
	}
	else if (cd == 2)
	{
		for (j = REGION_NUM_RECTS(&clip_reg) - 1; j >= 0; j--)
		{
			box = REGION_RECTS(&clip_reg)[j];
			rdp_set_clip(box.x1, box.y1, (box.x2 - box.x1), (box.y2 - box.y1));
			rdp_send_area_update(pDst->x + x, pDst->y + y, w, h);
		}

		rdp_reset_clip();
	}

	RegionUninit(&clip_reg);
}

/**
 * CopyArea
 */

static RegionPtr rdpCopyAreaOrg(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		int srcx, int srcy, int w, int h, int dstx, int dsty)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;
	RegionPtr rv;

	GC_OP_PROLOGUE(pGC);
	rv = pGC->ops->CopyArea(pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty);
	GC_OP_EPILOGUE(pGC);
	return rv;
}

static RegionPtr rdpCopyAreaWndToWnd(WindowPtr pSrcWnd, WindowPtr pDstWnd, GCPtr pGC,
		int srcx, int srcy, int w, int h, int dstx, int dsty)
{
	int cd;
	int lsrcx;
	int lsrcy;
	int ldstx;
	int ldsty;
	int num_clips;
	int dx;
	int dy;
	int j;
	BoxRec box;
	RegionPtr rv;
	RegionRec clip_reg;
	RDS_MSG_SCREEN_BLT msg;

	rv = rdpCopyAreaOrg(&(pSrcWnd->drawable), &(pDstWnd->drawable),
			pGC, srcx, srcy, w, h, dstx, dsty);

	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, &(pDstWnd->drawable), pGC);
	lsrcx = pSrcWnd->drawable.x + srcx;
	lsrcy = pSrcWnd->drawable.y + srcy;
	ldstx = pDstWnd->drawable.x + dstx;
	ldsty = pDstWnd->drawable.y + dsty;

	if (cd == 1)
	{
		msg.type = RDS_SERVER_SCREEN_BLT;
		msg.nLeftRect = ldstx;
		msg.nTopRect = ldsty;
		msg.nWidth = w;
		msg.nHeight = h;
		msg.nXSrc = lsrcx;
		msg.nYSrc = lsrcy;

		rdp_send_update((RDS_MSG_COMMON*) &msg);
	}
	else if (cd == 2)
	{
		num_clips = REGION_NUM_RECTS(&clip_reg);

		if (num_clips > 0)
		{
			dx = dstx - srcx;
			dy = dsty - srcy;

			if ((dy < 0) || ((dy == 0) && (dx < 0)))
			{
				for (j = 0; j < num_clips; j++)
				{
					box = REGION_RECTS(&clip_reg)[j];
					rdp_set_clip(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);

					msg.type = RDS_SERVER_SCREEN_BLT;
					msg.nLeftRect = ldstx;
					msg.nTopRect = ldsty;
					msg.nWidth = w;
					msg.nHeight = h;
					msg.nXSrc = lsrcx;
					msg.nYSrc = lsrcy;

					rdp_send_update((RDS_MSG_COMMON*) &msg);
				}
			}
			else
			{
				for (j = num_clips - 1; j >= 0; j--)
				{
					box = REGION_RECTS(&clip_reg)[j];
					rdp_set_clip(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);

					msg.type = RDS_SERVER_SCREEN_BLT;
					msg.nLeftRect = ldstx;
					msg.nTopRect = ldsty;
					msg.nWidth = w;
					msg.nHeight = h;
					msg.nXSrc = lsrcx;
					msg.nYSrc = lsrcy;

					rdp_send_update((RDS_MSG_COMMON*) &msg);
				}
			}

			rdp_reset_clip();
		}
	}

	RegionUninit(&clip_reg);

	return rv;
}

RegionPtr rdpCopyArea(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC, int srcx, int srcy, int w, int h, int dstx, int dsty)
{
	RegionPtr rv;
	RegionRec clip_reg;
	RegionRec box_reg;
	int num_clips;
	int cd;
	int j;
	int can_do_screen_blt;
	int post_process;
	BoxRec box;
	BoxPtr pbox;
	PixmapPtr pSrcPixmap;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pSrcPriv;
	rdpPixmapRec *pDstPriv;
	WindowPtr pDstWnd;
	WindowPtr pSrcWnd;

	if (pSrc->type == DRAWABLE_WINDOW)
	{
		pSrcWnd = (WindowPtr) pSrc;

		if (pSrcWnd->viewable)
		{
			if (pDst->type == DRAWABLE_WINDOW)
			{
				pDstWnd = (WindowPtr) pDst;

				if (pDstWnd->viewable)
				{
					can_do_screen_blt = pGC->alu == GXcopy;

					if (can_do_screen_blt)
					{
						return rdpCopyAreaWndToWnd(pSrcWnd, pDstWnd, pGC, srcx, srcy, w, h, dstx, dsty);
					}
				}
			}
			else if (pDst->type == DRAWABLE_PIXMAP)
			{
				pDstPixmap = (PixmapPtr) pDst;
				pDstPriv = GETPIXPRIV(pDstPixmap);
			}
		}
	}

	if (pSrc->type == DRAWABLE_PIXMAP)
	{
		pSrcPixmap = (PixmapPtr) pSrc;
		pSrcPriv = GETPIXPRIV(pSrcPixmap);
	}

	/* do original call */
	rv = rdpCopyAreaOrg(pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty);

	post_process = 0;

	if (pDst->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDst;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDst->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDst;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
		return rv;

	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, pDst, pGC);

	if (cd == 1)
	{
		rdp_send_area_update(pDst->x + dstx, pDst->y + dsty, w, h);
	}
	else if (cd == 2)
	{
		num_clips = REGION_NUM_RECTS(&clip_reg);

		if (num_clips > 0)
		{
			box.x1 = pDst->x + dstx;
			box.y1 = pDst->y + dsty;
			box.x2 = box.x1 + w;
			box.y2 = box.y1 + h;
			RegionInit(&box_reg, &box, 0);
			RegionIntersect(&clip_reg, &clip_reg, &box_reg);
			num_clips = REGION_NUM_RECTS(&clip_reg);

			if (num_clips < 10)
			{
				for (j = num_clips - 1; j >= 0; j--)
				{
					box = REGION_RECTS(&clip_reg)[j];
					rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1,
							box.y2 - box.y1);
				}
			}
			else
			{
				pbox = RegionExtents(&clip_reg);
				rdp_send_area_update(pbox->x1, pbox->y1, pbox->x2 - pbox->x1,
						pbox->y2 - pbox->y1);
			}

			RegionUninit(&box_reg);
		}
	}

	RegionUninit(&clip_reg);

	return rv;
}

/**
 * CopyPlane
 */

RegionPtr rdpCopyPlaneOrg(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		int srcx, int srcy, int w, int h, int dstx, int dsty, unsigned long bitPlane)
{
	RegionPtr rv;
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	rv = pGC->ops->CopyPlane(pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty, bitPlane);
	GC_OP_EPILOGUE(pGC);

	return rv;
}

RegionPtr rdpCopyPlane(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		int srcx, int srcy, int w, int h, int dstx, int dsty, unsigned long bitPlane)
{
	RegionPtr rv;
	RegionRec clip_reg;
	RegionRec box_reg;
	int cd;
	int num_clips;
	int j;
	int post_process;
	BoxRec box;
	BoxPtr pbox;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	/* do original call */
	rv = rdpCopyPlaneOrg(pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty, bitPlane);

	post_process = 0;

	if (pDst->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDst;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDst->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDst;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
		return rv;

	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, pDst, pGC);

	if (cd == 1)
	{
		rdp_send_area_update(pDst->x + dstx, pDst->y + dsty, w, h);
	}
	else if (cd == 2)
	{
		num_clips = REGION_NUM_RECTS(&clip_reg);

		if (num_clips > 0)
		{
			box.x1 = pDst->x + dstx;
			box.y1 = pDst->y + dsty;
			box.x2 = box.x1 + w;
			box.y2 = box.y1 + h;
			RegionInit(&box_reg, &box, 0);
			RegionIntersect(&clip_reg, &clip_reg, &box_reg);
			num_clips = REGION_NUM_RECTS(&clip_reg);

			if (num_clips < 10)
			{
				for (j = num_clips - 1; j >= 0; j--)
				{
					box = REGION_RECTS(&clip_reg)[j];
					rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
				}
			}
			else
			{
				pbox = RegionExtents(&clip_reg);
				rdp_send_area_update(pbox->x1, pbox->y1, pbox->x2 - pbox->x1, pbox->y2 - pbox->y1);
			}

			RegionUninit(&box_reg);
		}
	}

	RegionUninit(&clip_reg);

	return rv;
}

/**
 * PolyPoint
 */

void rdpPolyPointOrg(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt, DDXPointPtr in_pts)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->PolyPoint(pDrawable, pGC, mode, npt, in_pts);
	GC_OP_EPILOGUE(pGC);
}

void rdpPolyPoint(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt, DDXPointPtr in_pts)
{
	RegionRec clip_reg;
	int num_clips;
	int cd;
	int i;
	int j;
	int post_process;
	BoxRec box;
	BoxRec total_box;
	DDXPointPtr pts;
	DDXPointRec stack_pts[32];
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	if (npt > 32)
	{
		pts = (DDXPointPtr) malloc(sizeof(DDXPointRec) * npt);
	}
	else
	{
		pts = stack_pts;
	}

	for (i = 0; i < npt; i++)
	{
		pts[i].x = pDrawable->x + in_pts[i].x;
		pts[i].y = pDrawable->y + in_pts[i].y;

		if (i == 0)
		{
			total_box.x1 = pts[0].x;
			total_box.y1 = pts[0].y;
			total_box.x2 = pts[0].x;
			total_box.y2 = pts[0].y;
		}
		else
		{
			if (pts[i].x < total_box.x1)
			{
				total_box.x1 = pts[i].x;
			}

			if (pts[i].y < total_box.y1)
			{
				total_box.y1 = pts[i].y;
			}

			if (pts[i].x > total_box.x2)
			{
				total_box.x2 = pts[i].x;
			}

			if (pts[i].y > total_box.y2)
			{
				total_box.y2 = pts[i].y;
			}
		}

		/* todo, use this total_box */
	}

	/* do original call */
	rdpPolyPointOrg(pDrawable, pGC, mode, npt, in_pts);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
		return;

	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, pDrawable, pGC);

	if (cd == 1)
	{
		if (npt > 0)
		{
			RDS_MSG_OPAQUE_RECT msg;

			for (i = 0; i < npt; i++)
			{
				msg.type = RDS_SERVER_OPAQUE_RECT;
				msg.nLeftRect = pts[i].x;
				msg.nTopRect = pts[i].y;
				msg.nWidth = 1;
				msg.nHeight = 1;
				msg.color = rdp_convert_color(pGC->fgPixel);

				rdp_send_update((RDS_MSG_COMMON*) &msg);
			}
		}
	}
	else if (cd == 2)
	{
		num_clips = REGION_NUM_RECTS(&clip_reg);

		if (npt > 0 && num_clips > 0)
		{
			RDS_MSG_OPAQUE_RECT msg;

			for (j = num_clips - 1; j >= 0; j--)
			{
				box = REGION_RECTS(&clip_reg)[j];
				rdp_set_clip(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);

				for (i = 0; i < npt; i++)
				{
					msg.type = RDS_SERVER_OPAQUE_RECT;
					msg.nLeftRect = pts[i].x;
					msg.nTopRect = pts[i].y;
					msg.nWidth = 1;
					msg.nHeight = 1;
					msg.color = rdp_convert_color(pGC->fgPixel);

					rdp_send_update((RDS_MSG_COMMON*) &msg);
				}
			}

			rdp_reset_clip();
		}
	}

	RegionUninit(&clip_reg);

	if (pts != stack_pts)
	{
		free(pts);
	}
}

/**
 * Polylines
 */

void rdpPolylinesOrg(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt, DDXPointPtr pptInit)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->Polylines(pDrawable, pGC, mode, npt, pptInit);
	GC_OP_EPILOGUE(pGC);
}

void rdpPolylines(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt, DDXPointPtr pptInit)
{
	RegionRec clip_reg;
	int num_clips;
	int cd;
	int i;
	int j;
	int post_process;
	BoxRec box;
	xSegment *segs;
	int nseg;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	/* convert lines to line segments */
	nseg = npt - 1;
	segs = 0;

	if (npt > 1)
	{
		segs = (xSegment*) malloc(sizeof(xSegment) * nseg);

		segs[0].x1 = pptInit[0].x + pDrawable->x;
		segs[0].y1 = pptInit[0].y + pDrawable->y;

		if (mode == CoordModeOrigin)
		{
			segs[0].x2 = pptInit[1].x + pDrawable->x;
			segs[0].y2 = pptInit[1].y + pDrawable->y;

			for (i = 2; i < npt; i++)
			{
				segs[i - 1].x1 = segs[i - 2].x2;
				segs[i - 1].y1 = segs[i - 2].y2;
				segs[i - 1].x2 = pptInit[i].x + pDrawable->x;
				segs[i - 1].y2 = pptInit[i].y + pDrawable->y;
			}
		}
		else
		{
			segs[0].x2 = segs[0].x1 + pptInit[1].x;
			segs[0].y2 = segs[0].y1 + pptInit[1].y;

			for (i = 2; i < npt; i++)
			{
				segs[i - 1].x1 = segs[i - 2].x2;
				segs[i - 1].y1 = segs[i - 2].y2;
				segs[i - 1].x2 = segs[i - 1].x1 + pptInit[i].x;
				segs[i - 1].y2 = segs[i - 1].y1 + pptInit[i].y;
			}
		}
	}
	else
	{

	}

	/* do original call */
	rdpPolylinesOrg(pDrawable, pGC, mode, npt, pptInit);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
	{
		free(segs);
		return;
	}

	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, pDrawable, pGC);

	if (cd == 1)
	{
		if (segs != 0)
		{
			RDS_MSG_LINE_TO msg;

			msg.type = RDS_SERVER_LINE_TO;
			msg.bRop2 = rdp_convert_opcode(pGC->alu);
			msg.penColor = rdp_convert_color(pGC->fgPixel);
			msg.penWidth = pGC->lineWidth;
			msg.penStyle = 0;

			for (i = 0; i < nseg; i++)
			{
				msg.nXStart = segs[i].x1;
				msg.nYStart = segs[i].y1;
				msg.nXEnd = segs[i].x2;
				msg.nYEnd = segs[i].y2;

				rdp_send_update((RDS_MSG_COMMON*) &msg);
			}
		}
	}
	else if (cd == 2)
	{
		num_clips = REGION_NUM_RECTS(&clip_reg);

		if (nseg != 0 && num_clips > 0)
		{
			RDS_MSG_LINE_TO msg;

			msg.type = RDS_SERVER_LINE_TO;
			msg.bRop2 = rdp_convert_opcode(pGC->alu);
			msg.penColor = rdp_convert_color(pGC->fgPixel);
			msg.penWidth = pGC->lineWidth;
			msg.penStyle = 0;

			for (j = num_clips - 1; j >= 0; j--)
			{
				box = REGION_RECTS(&clip_reg)[j];
				rdp_set_clip(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);

				for (i = 0; i < nseg; i++)
				{
					msg.nXStart = segs[i].x1;
					msg.nYStart = segs[i].y1;
					msg.nXEnd = segs[i].x2;
					msg.nYEnd = segs[i].y2;

					rdp_send_update((RDS_MSG_COMMON*) &msg);
				}
			}

			rdp_reset_clip();
		}
	}

	free(segs);
	RegionUninit(&clip_reg);
}

/**
 * PolySegment
 */

void rdpPolySegmentOrg(DrawablePtr pDrawable, GCPtr pGC, int nseg, xSegment *pSegs)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->PolySegment(pDrawable, pGC, nseg, pSegs);
	GC_OP_EPILOGUE(pGC);
}

void rdpPolySegment(DrawablePtr pDrawable, GCPtr pGC, int nseg, xSegment *pSegs)
{
	RegionRec clip_reg;
	int cd;
	int i;
	int j;
	int post_process;
	xSegment *segs;
	BoxRec box;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	segs = 0;

	if (nseg) /* get the rects */
	{
		segs = (xSegment*) malloc(nseg * sizeof(xSegment));

		for (i = 0; i < nseg; i++)
		{
			segs[i].x1 = pSegs[i].x1 + pDrawable->x;
			segs[i].y1 = pSegs[i].y1 + pDrawable->y;
			segs[i].x2 = pSegs[i].x2 + pDrawable->x;
			segs[i].y2 = pSegs[i].y2 + pDrawable->y;
		}
	}

	/* do original call */
	rdpPolySegmentOrg(pDrawable, pGC, nseg, pSegs);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
	{
		free(segs);
		return;
	}

	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, pDrawable, pGC);

	if (cd == 1) /* no clip */
	{
		if (segs != 0)
		{
			RDS_MSG_LINE_TO msg;

			msg.type = RDS_SERVER_LINE_TO;
			msg.bRop2 = rdp_convert_opcode(pGC->alu);
			msg.penColor = rdp_convert_color(pGC->fgPixel);
			msg.penWidth = pGC->lineWidth;
			msg.penStyle = 0;

			for (i = 0; i < nseg; i++)
			{
				msg.nXStart = segs[i].x1;
				msg.nYStart = segs[i].y1;
				msg.nXEnd = segs[i].x2;
				msg.nYEnd = segs[i].y2;

				rdp_send_update((RDS_MSG_COMMON*) &msg);
			}
		}
	}
	else if (cd == 2) /* clip */
	{
		if (segs != 0)
		{
			RDS_MSG_LINE_TO msg;

			msg.type = RDS_SERVER_LINE_TO;
			msg.bRop2 = rdp_convert_opcode(pGC->alu);
			msg.penColor = rdp_convert_color(pGC->fgPixel);
			msg.penWidth = pGC->lineWidth;
			msg.penStyle = 0;

			for (j = REGION_NUM_RECTS(&clip_reg) - 1; j >= 0; j--)
			{
				box = REGION_RECTS(&clip_reg)[j];
				rdp_set_clip(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);

				for (i = 0; i < nseg; i++)
				{
					msg.nXStart = segs[i].x1;
					msg.nYStart = segs[i].y1;
					msg.nXEnd = segs[i].x2;
					msg.nYEnd = segs[i].y2;

					rdp_send_update((RDS_MSG_COMMON*) &msg);
				}
			}

			rdp_reset_clip();
		}
	}

	free(segs);
	RegionUninit(&clip_reg);
}

/**
 * PolyRectangle
 */

void rdpPolyRectangleOrg(DrawablePtr pDrawable, GCPtr pGC, int nrects, xRectangle *rects)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->PolyRectangle(pDrawable, pGC, nrects, rects);
	GC_OP_EPILOGUE(pGC);
}

/* tested with pGC->lineWidth = 0, 1, 2, 4 and opcodes 3 and 6 */
void rdpPolyRectangle(DrawablePtr pDrawable, GCPtr pGC, int nrects, xRectangle *rects)
{
	RegionRec clip_reg;
	RegionPtr fill_reg;
	int num_clips;
	int cd;
	int lw;
	int i;
	int j;
	int up;
	int down;
	int post_process;
	xRectangle *regRects;
	xRectangle *r;
	xRectangle *rect1;
	BoxRec box;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec* pDstPriv;

	/* make a copy of rects */
	rect1 = (xRectangle*) malloc(sizeof(xRectangle) * nrects);

	for (i = 0; i < nrects; i++)
	{
		rect1[i] = rects[i];
	}

	/* do original call */
	rdpPolyRectangleOrg(pDrawable, pGC, nrects, rects);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
	{
		free(rect1);
		return;
	}

	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, pDrawable, pGC);
	regRects = 0;

	if ((cd != 0) && (nrects > 0))
	{
		regRects = (xRectangle*) malloc(nrects * 4 * sizeof(xRectangle));

		lw = pGC->lineWidth;

		if (lw < 1)
		{
			lw = 1;
		}

		up = lw / 2;
		down = 1 + (lw - 1) / 2;

		for (i = 0; i < nrects; i++)
		{
			r = regRects + i * 4;
			r->x = (rect1[i].x + pDrawable->x) - up;
			r->y = (rect1[i].y + pDrawable->y) - up;
			r->width = rect1[i].width + up + down;
			r->height = lw;
			r++;
			r->x = (rect1[i].x + pDrawable->x) - up;
			r->y = (rect1[i].y + pDrawable->y) + down;
			r->width = lw;
			r->height = MAX(rect1[i].height - (up + down), 0);
			r++;
			r->x = ((rect1[i].x + rect1[i].width) + pDrawable->x) - up;
			r->y = (rect1[i].y + pDrawable->y) + down;
			r->width = lw;
			r->height = MAX(rect1[i].height - (up + down), 0);
			r++;
			r->x = (rect1[i].x + pDrawable->x) - up;
			r->y = ((rect1[i].y + rect1[i].height) + pDrawable->y) - up;
			r->width = rect1[i].width + up + down;
			r->height = lw;
		}
	}

	if (cd == 1)
	{
		if (regRects != 0)
		{
			RDS_MSG_OPAQUE_RECT msg;

			if (pGC->lineStyle == LineSolid)
			{
				for (i = 0; i < nrects * 4; i++)
				{
					r = regRects + i;

					msg.type = RDS_SERVER_OPAQUE_RECT;
					msg.nLeftRect = r->x;
					msg.nTopRect = r->y;
					msg.nWidth = r->width;
					msg.nHeight = r->height;
					msg.color = rdp_convert_color(pGC->fgPixel);

					rdp_send_update((RDS_MSG_COMMON*) &msg);
				}
			}
			else
			{
				for (i = 0; i < nrects * 4; i++)
				{
					r = regRects + i;
					rdp_send_area_update(r->x, r->y, r->width, r->height);
				}
			}
		}
	}
	else if (cd == 2)
	{
		if (regRects != 0)
		{
			fill_reg = RegionFromRects(nrects * 4, regRects, CT_NONE);
			RegionIntersect(&clip_reg, &clip_reg, fill_reg);
			num_clips = REGION_NUM_RECTS(&clip_reg);

			if (num_clips > 0)
			{
				if (pGC->lineStyle == LineSolid)
				{
					RDS_MSG_OPAQUE_RECT msg;

					for (j = num_clips - 1; j >= 0; j--)
					{
						box = REGION_RECTS(&clip_reg)[j];

						msg.type = RDS_SERVER_OPAQUE_RECT;
						msg.nLeftRect = box.x1;
						msg.nTopRect = box.y1;
						msg.nWidth = box.x2 - box.x1;
						msg.nHeight = box.y2 - box.y1;
						msg.color = rdp_convert_color(pGC->fgPixel);

						rdp_send_update((RDS_MSG_COMMON*) &msg);
					}
				}
				else
				{
					for (j = num_clips - 1; j >= 0; j--)
					{
						box = REGION_RECTS(&clip_reg)[j];
						rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
					}
				}
			}

			RegionDestroy(fill_reg);
		}
	}

	RegionUninit(&clip_reg);
	free(regRects);
	free(rect1);
}

/**
 * PolyArc
 */

void rdpPolyArcOrg(DrawablePtr pDrawable, GCPtr pGC, int narcs, xArc *parcs)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->PolyArc(pDrawable, pGC, narcs, parcs);
	GC_OP_EPILOGUE(pGC);
}

void rdpPolyArc(DrawablePtr pDrawable, GCPtr pGC, int narcs, xArc *parcs)
{
	RegionRec clip_reg;
	RegionPtr tmpRegion;
	int cd;
	int lw;
	int extra;
	int i;
	int num_clips;
	int post_process;
	xRectangle *rects;
	BoxRec box;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	rects = 0;

	if (narcs > 0)
	{
		rects = (xRectangle*) malloc(narcs * sizeof(xRectangle));

		lw = pGC->lineWidth;

		if (lw == 0)
		{
			lw = 1;
		}

		extra = lw / 2;

		for (i = 0; i < narcs; i++)
		{
			rects[i].x = (parcs[i].x - extra) + pDrawable->x;
			rects[i].y = (parcs[i].y - extra) + pDrawable->y;
			rects[i].width = parcs[i].width + lw;
			rects[i].height = parcs[i].height + lw;
		}
	}

	/* do original call */
	rdpPolyArcOrg(pDrawable, pGC, narcs, parcs);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
	{
		free(rects);
		return;
	}

	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, pDrawable, pGC);

	if (cd == 1)
	{
		if (rects != 0)
		{
			tmpRegion = RegionFromRects(narcs, rects, CT_NONE);
			num_clips = REGION_NUM_RECTS(tmpRegion);

			if (num_clips > 0)
			{
				for (i = num_clips - 1; i >= 0; i--)
				{
					box = REGION_RECTS(tmpRegion)[i];
					rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1,
							box.y2 - box.y1);
				}
			}

			RegionDestroy(tmpRegion);
		}
	}
	else if (cd == 2)
	{
		if (rects != 0)
		{
			tmpRegion = RegionFromRects(narcs, rects, CT_NONE);
			RegionIntersect(tmpRegion, tmpRegion, &clip_reg);
			num_clips = REGION_NUM_RECTS(tmpRegion);

			if (num_clips > 0)
			{
				for (i = num_clips - 1; i >= 0; i--)
				{
					box = REGION_RECTS(tmpRegion)[i];
					rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1,
							box.y2 - box.y1);
				}
			}

			RegionDestroy(tmpRegion);
		}
	}

	RegionUninit(&clip_reg);
	free(rects);
}

void rdpFillPolygonOrg(DrawablePtr pDrawable, GCPtr pGC, int shape, int mode, int count, DDXPointPtr pPts)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->FillPolygon(pDrawable, pGC, shape, mode, count, pPts);
	GC_OP_EPILOGUE(pGC);
}

void rdpFillPolygon(DrawablePtr pDrawable, GCPtr pGC, int shape, int mode, int count, DDXPointPtr pPts)
{
	RegionRec clip_reg;
	RegionRec box_reg;
	int num_clips;
	int cd;
	int maxx;
	int maxy;
	int minx;
	int miny;
	int i;
	int j;
	int post_process;
	BoxRec box;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	box.x1 = 0;
	box.y1 = 0;
	box.x2 = 0;
	box.y2 = 0;

	if (count > 0)
	{
		maxx = pPts[0].x;
		maxy = pPts[0].y;
		minx = maxx;
		miny = maxy;

		for (i = 1; i < count; i++)
		{
			if (pPts[i].x > maxx)
			{
				maxx = pPts[i].x;
			}

			if (pPts[i].x < minx)
			{
				minx = pPts[i].x;
			}

			if (pPts[i].y > maxy)
			{
				maxy = pPts[i].y;
			}

			if (pPts[i].y < miny)
			{
				miny = pPts[i].y;
			}
		}

		box.x1 = pDrawable->x + minx;
		box.y1 = pDrawable->y + miny;
		box.x2 = pDrawable->x + maxx + 1;
		box.y2 = pDrawable->y + maxy + 1;
	}

	/* do original call */
	rdpFillPolygonOrg(pDrawable, pGC, shape, mode, count, pPts);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
		return;

	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, pDrawable, pGC);

	if (cd == 1)
	{
		rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
	}
	else if (cd == 2)
	{
		RegionInit(&box_reg, &box, 0);
		RegionIntersect(&clip_reg, &clip_reg, &box_reg);
		num_clips = REGION_NUM_RECTS(&clip_reg);

		if (num_clips > 0)
		{
			for (j = num_clips - 1; j >= 0; j--)
			{
				box = REGION_RECTS(&clip_reg)[j];
				rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
			}
		}

		RegionUninit(&box_reg);
	}

	RegionUninit(&clip_reg);
}

/**
 * PolyFillRect
 */

static void rdpPolyFillRectOrg(DrawablePtr pDrawable, GCPtr pGC, int nrectFill, xRectangle *prectInit)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->PolyFillRect(pDrawable, pGC, nrectFill, prectInit);
	GC_OP_EPILOGUE(pGC);
}

void rdpPolyFillRect(DrawablePtr pDrawable, GCPtr pGC, int nrectFill, xRectangle* prectInit)
{
	int j;
	int cd;
	int num_clips;
	RegionRec clip_reg;
	RegionPtr fill_reg;
	BoxRec box;
	int post_process;
	UINT32 dstblt_rop;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec* pDstPriv;

	/* make a copy of rects */
	fill_reg = RegionFromRects(nrectFill, prectInit, CT_NONE);

	/* do original call */
	rdpPolyFillRectOrg(pDrawable, pGC, nrectFill, prectInit);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
	{
		RegionDestroy(fill_reg);
		return;
	}

	RegionTranslate(fill_reg, pDrawable->x, pDrawable->y);
	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, pDrawable, pGC);

	if (cd == 1) /* no clip */
	{
		dstblt_rop = rdp_dstblt_rop(pGC->alu);

		if ((pGC->fillStyle == 0) && (dstblt_rop))
		{
			RDS_MSG_DSTBLT msg;

			for (j = REGION_NUM_RECTS(fill_reg) - 1; j >= 0; j--)
			{
				box = REGION_RECTS(fill_reg)[j];

				msg.type = RDS_SERVER_DSTBLT;
				msg.nLeftRect = box.x1;
				msg.nTopRect = box.y1;
				msg.nWidth = box.x2 - box.x1;
				msg.nHeight = box.y2 - box.y1;
				msg.bRop = dstblt_rop;

				rdp_send_update((RDS_MSG_COMMON*) &msg);
			}
		}
		else
		{
			for (j = REGION_NUM_RECTS(fill_reg) - 1; j >= 0; j--)
			{
				box = REGION_RECTS(fill_reg)[j];
				rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
			}
		}
	}
	else if (cd == 2) /* clip */
	{
		RegionIntersect(&clip_reg, &clip_reg, fill_reg);
		num_clips = REGION_NUM_RECTS(&clip_reg);

		if (num_clips > 0)
		{
			dstblt_rop = rdp_dstblt_rop(pGC->alu);

			if ((pGC->fillStyle == 0) && (dstblt_rop))
			{
				RDS_MSG_DSTBLT msg;

				for (j = num_clips - 1; j >= 0; j--)
				{
					box = REGION_RECTS(&clip_reg)[j];

					msg.type = RDS_SERVER_DSTBLT;
					msg.nLeftRect = box.x1;
					msg.nTopRect = box.y1;
					msg.nWidth = box.x2 - box.x1;
					msg.nHeight = box.y2 - box.y1;
					msg.bRop = dstblt_rop;

					rdp_send_update((RDS_MSG_COMMON*) &msg);
				}
			}
			else
			{
				for (j = num_clips - 1; j >= 0; j--)
				{
					box = REGION_RECTS(&clip_reg)[j];
					rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
				}
			}
		}
	}

	RegionUninit(&clip_reg);
	RegionDestroy(fill_reg);
}

/**
 * PolyFillArc
 */

void rdpPolyFillArcOrg(DrawablePtr pDrawable, GCPtr pGC, int narcs, xArc *parcs)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->PolyFillArc(pDrawable, pGC, narcs, parcs);
	GC_OP_EPILOGUE(pGC);
}

void rdpPolyFillArc(DrawablePtr pDrawable, GCPtr pGC, int narcs, xArc *parcs)
{
	RegionRec clip_reg;
	RegionPtr tmpRegion;
	int cd;
	int lw;
	int extra;
	int i;
	int num_clips;
	int post_process;
	xRectangle *rects;
	BoxRec box;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	rects = 0;

	if (narcs > 0)
	{
		rects = (xRectangle*) malloc(narcs * sizeof(xRectangle));

		lw = pGC->lineWidth;

		if (lw == 0)
		{
			lw = 1;
		}

		extra = lw / 2;

		for (i = 0; i < narcs; i++)
		{
			rects[i].x = (parcs[i].x - extra) + pDrawable->x;
			rects[i].y = (parcs[i].y - extra) + pDrawable->y;
			rects[i].width = parcs[i].width + lw;
			rects[i].height = parcs[i].height + lw;
		}
	}

	/* do original call */
	rdpPolyFillArcOrg(pDrawable, pGC, narcs, parcs);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
	{
		free(rects);
		return;
	}

	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, pDrawable, pGC);

	if (cd == 1)
	{
		if (rects != 0)
		{
			tmpRegion = RegionFromRects(narcs, rects, CT_NONE);
			num_clips = REGION_NUM_RECTS(tmpRegion);

			if (num_clips > 0)
			{
				for (i = num_clips - 1; i >= 0; i--)
				{
					box = REGION_RECTS(tmpRegion)[i];
					rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1,
							box.y2 - box.y1);
				}
			}

			RegionDestroy(tmpRegion);
		}
	}
	else if (cd == 2)
	{
		if (rects != 0)
		{
			tmpRegion = RegionFromRects(narcs, rects, CT_NONE);
			RegionIntersect(tmpRegion, tmpRegion, &clip_reg);
			num_clips = REGION_NUM_RECTS(tmpRegion);

			if (num_clips > 0)
			{
				for (i = num_clips - 1; i >= 0; i--)
				{
					box = REGION_RECTS(tmpRegion)[i];
					rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1,
							box.y2 - box.y1);
				}
			}

			RegionDestroy(tmpRegion);
		}
	}

	RegionUninit(&clip_reg);
	free(rects);
}

/**
 * PolyText8
 */

int rdpPolyText8Org(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count, char *chars)
{
	int rv;
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	rv = pGC->ops->PolyText8(pDrawable, pGC, x, y, count, chars);
	GC_OP_EPILOGUE(pGC);
	return rv;
}

int rdpPolyText8(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count, char *chars)
{
	RegionRec reg;
	RegionRec reg1;
	int num_clips;
	int cd;
	int j;
	int rv;
	int post_process;
	BoxRec box;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	if (count != 0)
	{
		GetTextBoundingBox(pDrawable, pGC->font, x, y, count, &box);
	}

	/* do original call */
	rv = rdpPolyText8Org(pDrawable, pGC, x, y, count, chars);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
		return rv;

	RegionInit(&reg, NullBox, 0);

	if (count == 0)
	{
		cd = 0;
	}
	else
	{
		cd = rdp_get_clip(&reg, pDrawable, pGC);
	}

	if (cd == 1)
	{
		rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
	}
	else if (cd == 2)
	{
		RegionInit(&reg1, &box, 0);
		RegionIntersect(&reg, &reg, &reg1);
		num_clips = REGION_NUM_RECTS(&reg);

		if (num_clips > 0)
		{
			for (j = num_clips - 1; j >= 0; j--)
			{
				box = REGION_RECTS(&reg)[j];
				rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
			}
		}

		RegionUninit(&reg1);
	}

	RegionUninit(&reg);

	return rv;
}

/**
 * PolyText16
 */

int rdpPolyText16Org(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count, unsigned short *chars)
{
	int rv;
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	rv = pGC->ops->PolyText16(pDrawable, pGC, x, y, count, chars);
	GC_OP_EPILOGUE(pGC);
	return rv;
}

int rdpPolyText16(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count, unsigned short *chars)
{
	RegionRec reg;
	RegionRec reg1;
	int num_clips;
	int cd;
	int j;
	int rv;
	int post_process;
	BoxRec box;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	if (count != 0)
	{
		GetTextBoundingBox(pDrawable, pGC->font, x, y, count, &box);
	}

	/* do original call */
	rv = rdpPolyText16Org(pDrawable, pGC, x, y, count, chars);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
		return rv;

	RegionInit(&reg, NullBox, 0);

	if (count == 0)
	{
		cd = 0;
	}
	else
	{
		cd = rdp_get_clip(&reg, pDrawable, pGC);
	}

	if (cd == 1)
	{
		rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
	}
	else if (cd == 2)
	{
		RegionInit(&reg1, &box, 0);
		RegionIntersect(&reg, &reg, &reg1);
		num_clips = REGION_NUM_RECTS(&reg);

		if (num_clips > 0)
		{
			for (j = num_clips - 1; j >= 0; j--)
			{
				box = REGION_RECTS(&reg)[j];
				rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
			}
		}

		RegionUninit(&reg1);
	}

	RegionUninit(&reg);

	return rv;
}

/**
 * ImageText8
 */

void rdpImageText8Org(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count, char *chars)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->ImageText8(pDrawable, pGC, x, y, count, chars);
	GC_OP_EPILOGUE(pGC);
}

void rdpImageText8(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count, char *chars)
{
	RegionRec reg;
	RegionRec reg1;
	int num_clips;
	int cd;
	int j;
	int post_process;
	BoxRec box;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	if (count != 0)
	{
		GetTextBoundingBox(pDrawable, pGC->font, x, y, count, &box);
	}

	/* do original call */
	rdpImageText8Org(pDrawable, pGC, x, y, count, chars);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
		return;

	RegionInit(&reg, NullBox, 0);

	if (count == 0)
	{
		cd = 0;
	}
	else
	{
		cd = rdp_get_clip(&reg, pDrawable, pGC);
	}

	if (cd == 1)
	{
		rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
	}
	else if (cd == 2)
	{
		RegionInit(&reg1, &box, 0);
		RegionIntersect(&reg, &reg, &reg1);
		num_clips = REGION_NUM_RECTS(&reg);

		if (num_clips > 0)
		{
			for (j = num_clips - 1; j >= 0; j--)
			{
				box = REGION_RECTS(&reg)[j];
				rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1,
						box.y2 - box.y1);
			}
		}

		RegionUninit(&reg1);
	}

	RegionUninit(&reg);

	return;
}

/**
 * ImageText16
 */

void rdpImageText16Org(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count, unsigned short *chars)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->ImageText16(pDrawable, pGC, x, y, count, chars);
	GC_OP_EPILOGUE(pGC);
}

void rdpImageText16(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count, unsigned short *chars)
{
	RegionRec reg;
	RegionRec reg1;
	int num_clips;
	int cd;
	int j;
	int post_process;
	BoxRec box;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	if (count != 0)
	{
		GetTextBoundingBox(pDrawable, pGC->font, x, y, count, &box);
	}

	/* do original call */
	rdpImageText16Org(pDrawable, pGC, x, y, count, chars);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
		return;

	RegionInit(&reg, NullBox, 0);

	if (count == 0)
	{
		cd = 0;
	}
	else
	{
		cd = rdp_get_clip(&reg, pDrawable, pGC);
	}

	if (cd == 1)
	{
		rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
	}
	else if (cd == 2)
	{
		RegionInit(&reg1, &box, 0);
		RegionIntersect(&reg, &reg, &reg1);
		num_clips = REGION_NUM_RECTS(&reg);

		if (num_clips > 0)
		{
			for (j = num_clips - 1; j >= 0; j--)
			{
				box = REGION_RECTS(&reg)[j];
				rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1,
						box.y2 - box.y1);
			}
		}

		RegionUninit(&reg1);
	}

	RegionUninit(&reg);

	return;
}

/**
 * ImageGlyphBlt
 */

void rdpImageGlyphBltOrg(DrawablePtr pDrawable, GCPtr pGC, int x, int y, unsigned int nglyph, CharInfoPtr *ppci, pointer pglyphBase)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->ImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
	GC_OP_EPILOGUE(pGC);
}

void rdpImageGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x, int y, unsigned int nglyph, CharInfoPtr *ppci, pointer pglyphBase)
{
	RegionRec reg;
	RegionRec reg1;
	int num_clips;
	int cd;
	int j;
	int post_process;
	BoxRec box;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	if (nglyph != 0)
	{
		GetTextBoundingBox(pDrawable, pGC->font, x, y, nglyph, &box);
	}

	/* do original call */
	rdpImageGlyphBltOrg(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
		return;

	RegionInit(&reg, NullBox, 0);

	if (nglyph == 0)
	{
		cd = 0;
	}
	else
	{
		cd = rdp_get_clip(&reg, pDrawable, pGC);
	}

	if (cd == 1)
	{
		rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
	}
	else if (cd == 2)
	{
		RegionInit(&reg1, &box, 0);
		RegionIntersect(&reg, &reg, &reg1);
		num_clips = REGION_NUM_RECTS(&reg);

		if (num_clips > 0)
		{
			for (j = num_clips - 1; j >= 0; j--)
			{
				box = REGION_RECTS(&reg)[j];
				rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
			}
		}

		RegionUninit(&reg1);
	}

	RegionUninit(&reg);

	return;
}

/**
 * PolyGlyphBlt
 */

void rdpPolyGlyphBltOrg(DrawablePtr pDrawable, GCPtr pGC,
		int x, int y, unsigned int nglyph, CharInfoPtr *ppci, pointer pglyphBase)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->PolyGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
	GC_OP_EPILOGUE(pGC);
}

void rdpPolyGlyphBlt(DrawablePtr pDrawable, GCPtr pGC,
		int x, int y, unsigned int nglyph, CharInfoPtr *ppci, pointer pglyphBase)
{
	RegionRec reg;
	RegionRec reg1;
	int num_clips;
	int cd;
	int j;
	int post_process;
	BoxRec box;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	if (nglyph != 0)
	{
		GetTextBoundingBox(pDrawable, pGC->font, x, y, nglyph, &box);
	}

	/* do original call */
	rdpPolyGlyphBltOrg(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);

	post_process = 0;

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDrawable;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDrawable->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDrawable;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
		return;

	RegionInit(&reg, NullBox, 0);

	if (nglyph == 0)
	{
		cd = 0;
	}
	else
	{
		cd = rdp_get_clip(&reg, pDrawable, pGC);
	}

	if (cd == 1)
	{
		rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
	}
	else if (cd == 2)
	{
		RegionInit(&reg1, &box, 0);
		RegionIntersect(&reg, &reg, &reg1);
		num_clips = REGION_NUM_RECTS(&reg);

		if (num_clips > 0)
		{
			for (j = num_clips - 1; j >= 0; j--)
			{
				box = REGION_RECTS(&reg)[j];
				rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1,
						box.y2 - box.y1);
			}
		}

		RegionUninit(&reg1);
	}

	RegionUninit(&reg);

	return;
}

/**
 * PushPixels
 */

void rdpPushPixelsOrg(GCPtr pGC, PixmapPtr pBitMap, DrawablePtr pDst, int w, int h, int x, int y)
{
	rdpGCPtr priv;
	const GCFuncs* oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->PushPixels(pGC, pBitMap, pDst, w, h, x, y);
	GC_OP_EPILOGUE(pGC);
}

void rdpPushPixels(GCPtr pGC, PixmapPtr pBitMap, DrawablePtr pDst, int w, int h, int x, int y)
{
	RegionRec clip_reg;
	RegionRec box_reg;
	int num_clips;
	int cd;
	int j;
	int post_process;
	BoxRec box;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	/* do original call */
	rdpPushPixelsOrg(pGC, pBitMap, pDst, w, h, x, y);

	post_process = 0;

	if (pDst->type == DRAWABLE_PIXMAP)
	{
		pDstPixmap = (PixmapPtr) pDst;
		pDstPriv = GETPIXPRIV(pDstPixmap);
	}
	else
	{
		if (pDst->type == DRAWABLE_WINDOW)
		{
			pDstWnd = (WindowPtr) pDst;

			if (pDstWnd->viewable)
			{
				post_process = 1;
			}
		}
	}

	if (!post_process)
		return;

	memset(&box, 0, sizeof(box));
	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, pDst, pGC);

	if (cd == 1)
	{
		rdp_send_area_update(pDst->x + x, pDst->y + y, w, h);
	}
	else if (cd == 2)
	{
		box.x1 = pDst->x + x;
		box.y1 = pDst->y + y;
		box.x2 = box.x1 + w;
		box.y2 = box.y1 + h;
		RegionInit(&box_reg, &box, 0);
		RegionIntersect(&clip_reg, &clip_reg, &box_reg);
		num_clips = REGION_NUM_RECTS(&clip_reg);

		if (num_clips > 0)
		{
			for (j = num_clips - 1; j >= 0; j--)
			{
				box = REGION_RECTS(&clip_reg)[j];
				rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
			}
		}

		RegionUninit(&box_reg);
	}

	RegionUninit(&clip_reg);
}

/**
 * GCOps table
 */

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
