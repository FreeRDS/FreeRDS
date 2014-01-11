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

void rdpImageGlyphBltOrg(DrawablePtr pDrawable, GCPtr pGC, int x, int y, unsigned int nglyph, CharInfoPtr *ppci, pointer pglyphBase)
{
	rdpGCPtr priv;
	GCFuncs *oldFuncs;

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

	LLOGLN(10, ("rdpImageGlyphBlt:"));

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
