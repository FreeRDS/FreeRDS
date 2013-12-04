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

int rdpPolyText8Org(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count, char *chars)
{
	int rv;
	rdpGCPtr priv;
	GCFuncs *oldFuncs;

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

	LLOGLN(10, ("rdpPolyText8:"));

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
		rdpup_begin_update();
		rdpup_send_area(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
		rdpup_end_update();
	}
	else if (cd == 2)
	{
		RegionInit(&reg1, &box, 0);
		RegionIntersect(&reg, &reg, &reg1);
		num_clips = REGION_NUM_RECTS(&reg);

		if (num_clips > 0)
		{
			rdpup_begin_update();

			for (j = num_clips - 1; j >= 0; j--)
			{
				box = REGION_RECTS(&reg)[j];
				rdpup_send_area(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
			}

			rdpup_end_update();
		}

		RegionUninit(&reg1);
	}

	RegionUninit(&reg);

	return rv;
}
