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

void rdpPolySegmentOrg(DrawablePtr pDrawable, GCPtr pGC, int nseg, xSegment *pSegs)
{
	rdpGCPtr priv;
	GCFuncs *oldFuncs;

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
	int got_id;
	int post_process;
	xSegment *segs;
	BoxRec box;
	struct image_data id;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	LLOGLN(10, ("rdpPolySegment:"));
	LLOGLN(10, ("  nseg %d", nseg));

	segs = 0;

	if (nseg) /* get the rects */
	{
		segs = (xSegment*) g_malloc(nseg * sizeof(xSegment), 0);

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
	LLOGLN(10, ("rdpPolySegment: cd %d", cd));

	if (cd == 1) /* no clip */
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
	else if (cd == 2) /* clip */
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

				for (j = REGION_NUM_RECTS(&clip_reg) - 1; j >= 0; j--)
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
