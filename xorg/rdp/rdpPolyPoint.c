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

extern GCOps g_rdpGCOps;

void rdpPolyPointOrg(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt, DDXPointPtr in_pts)
{
	rdpGCPtr priv;
	GCFuncs *oldFuncs;

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

	LLOGLN(10, ("rdpPolyPoint:"));
	LLOGLN(10, ("rdpPolyPoint:  npt %d", npt));

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

			rdpup_begin_update();

			for (i = 0; i < npt; i++)
			{
				msg.nLeftRect = pts[i].x;
				msg.nTopRect = pts[i].y;
				msg.nWidth = 1;
				msg.nHeight = 1;
				msg.color = rdpup_convert_color(pGC->fgPixel);

				rdpup_opaque_rect(&msg);
			}

			rdpup_end_update();
		}
	}
	else if (cd == 2)
	{
		num_clips = REGION_NUM_RECTS(&clip_reg);

		if (npt > 0 && num_clips > 0)
		{
			RDS_MSG_OPAQUE_RECT msg;

			rdpup_begin_update();

			for (j = num_clips - 1; j >= 0; j--)
			{
				box = REGION_RECTS(&clip_reg)[j];
				rdpup_set_clip(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);

				for (i = 0; i < npt; i++)
				{
					msg.nLeftRect = pts[i].x;
					msg.nTopRect = pts[i].y;
					msg.nWidth = 1;
					msg.nHeight = 1;
					msg.color = rdpup_convert_color(pGC->fgPixel);

					rdpup_opaque_rect(&msg);
				}
			}

			rdpup_reset_clip();
			rdpup_end_update();
		}
	}

	RegionUninit(&clip_reg);

	if (pts != stack_pts)
	{
		free(pts);
	}
}
