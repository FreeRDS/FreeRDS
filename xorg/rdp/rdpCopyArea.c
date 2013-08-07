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

 */

#include "rdp.h"
#include "rdpdraw.h"

#define LDEBUG 0

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
		do { if (_level < LOG_LEVEL) { ErrorF _args ; } } while (0)
#define LLOGLN(_level, _args) \
		do { if (_level < LOG_LEVEL) { ErrorF _args ; ErrorF("\n"); } } while (0)

extern DevPrivateKeyRec g_rdpGCIndex;
extern DevPrivateKeyRec g_rdpPixmapIndex;
extern rdpPixmapRec g_screenPriv;

extern GCOps g_rdpGCOps;

static RegionPtr rdpCopyAreaOrg(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		int srcx, int srcy, int w, int h, int dstx, int dsty)
{
	rdpGCPtr priv;
	GCFuncs *oldFuncs;
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

	LLOGLN(10, ("rdpCopyAreaWndToWnd:"));

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
		rdpup_begin_update();
		rdpup_screen_blt(ldstx, ldsty, w, h, lsrcx, lsrcy);
		rdpup_end_update();
	}
	else if (cd == 2)
	{
		num_clips = REGION_NUM_RECTS(&clip_reg);

		if (num_clips > 0)
		{
			rdpup_begin_update();

			dx = dstx - srcx;
			dy = dsty - srcy;

			if ((dy < 0) || ((dy == 0) && (dx < 0)))
			{
				for (j = 0; j < num_clips; j++)
				{
					box = REGION_RECTS(&clip_reg)[j];
					rdpup_set_clip(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
					rdpup_screen_blt(ldstx, ldsty, w, h, lsrcx, lsrcy);
				}
			}
			else
			{
				for (j = num_clips - 1; j >= 0; j--)
				{
					box = REGION_RECTS(&clip_reg)[j];
					rdpup_set_clip(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
					rdpup_screen_blt(ldstx, ldsty, w, h, lsrcx, lsrcy);
				}
			}

			rdpup_reset_clip();
			rdpup_end_update();
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
	int got_id;
	int post_process;
	struct image_data id;
	BoxRec box;
	BoxPtr pbox;
	PixmapPtr pSrcPixmap;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pSrcPriv;
	rdpPixmapRec *pDstPriv;
	WindowPtr pDstWnd;
	WindowPtr pSrcWnd;

	LLOGLN(10, ("rdpCopyArea:"));

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
	got_id = 0;

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

				rdpup_get_screen_image_rect(&id);
				got_id = 1;
			}
		}
	}

	if (!post_process)
	{
		return rv;
	}

	RegionInit(&clip_reg, NullBox, 0);
	cd = rdp_get_clip(&clip_reg, pDst, pGC);

	if (cd == 1)
	{
		if (got_id)
		{
			rdpup_begin_update();
			rdpup_send_area(&id, pDst->x + dstx, pDst->y + dsty, w, h);
			rdpup_end_update();
		}
	}
	else if (cd == 2)
	{
		num_clips = REGION_NUM_RECTS(&clip_reg);

		if (num_clips > 0)
		{
			if (got_id)
			{
				rdpup_begin_update();
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
						rdpup_send_area(&id, box.x1, box.y1, box.x2 - box.x1,
								box.y2 - box.y1);
					}
				}
				else
				{
					pbox = RegionExtents(&clip_reg);
					rdpup_send_area(&id, pbox->x1, pbox->y1, pbox->x2 - pbox->x1,
							pbox->y2 - pbox->y1);
				}

				RegionUninit(&box_reg);
				rdpup_end_update();
			}
		}
	}

	RegionUninit(&clip_reg);

	return rv;
}
