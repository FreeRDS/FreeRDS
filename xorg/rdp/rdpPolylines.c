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

static void rdpPolylinesOrg(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt, DDXPointPtr pptInit)
{
	rdpGCPtr priv;
	GCFuncs *oldFuncs;

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
	int got_id;
	int post_process;
	BoxRec box;
	xSegment *segs;
	int nseg;
	struct image_data id;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	LLOGLN(10, ("rdpPolylines:"));
	LLOGLN(10, ("  npt %d mode %d x %d y %d", npt, mode,
			pDrawable->x, pDrawable->y));

	/* convert lines to line segments */
	nseg = npt - 1;
	segs = 0;

	if (npt > 1)
	{
		segs = (xSegment*) g_malloc(sizeof(xSegment) * nseg, 0);
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
		LLOGLN(0, ("rdpPolylines: weird npt [%d]", npt));
	}

	/* do original call */
	rdpPolylinesOrg(pDrawable, pGC, mode, npt, pptInit);

	post_process = 0;
	got_id = 0;

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

				rdpup_get_screen_image_rect(&id);
				got_id = 1;
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
			if (got_id)
			{
				XRDP_MSG_LINE_TO msg;

				rdpup_begin_update();

				msg.bRop2 = rdpup_convert_opcode(pGC->alu);
				msg.penColor = rdpup_convert_color(pGC->fgPixel);
				msg.penWidth = pGC->lineWidth;
				msg.penStyle = 0;

				for (i = 0; i < nseg; i++)
				{
					msg.nXStart = segs[i].x1;
					msg.nYStart = segs[i].y1;
					msg.nXEnd = segs[i].x2;
					msg.nYEnd = segs[i].y2;

					rdpup_draw_line(&msg);
				}

				rdpup_end_update();
			}
		}
	}
	else if (cd == 2)
	{
		num_clips = REGION_NUM_RECTS(&clip_reg);

		if (nseg != 0 && num_clips > 0)
		{
			if (got_id)
			{
				XRDP_MSG_LINE_TO msg;

				rdpup_begin_update();

				msg.bRop2 = rdpup_convert_opcode(pGC->alu);
				msg.penColor = rdpup_convert_color(pGC->fgPixel);
				msg.penWidth = pGC->lineWidth;
				msg.penStyle = 0;

				for (j = num_clips - 1; j >= 0; j--)
				{
					box = REGION_RECTS(&clip_reg)[j];
					rdpup_set_clip(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);

					for (i = 0; i < nseg; i++)
					{
						msg.nXStart = segs[i].x1;
						msg.nYStart = segs[i].y1;
						msg.nXEnd = segs[i].x2;
						msg.nYEnd = segs[i].y2;

						rdpup_draw_line(&msg);
					}
				}

				rdpup_reset_clip();
				rdpup_end_update();
			}
		}
	}

	free(segs);
	RegionUninit(&clip_reg);
}
