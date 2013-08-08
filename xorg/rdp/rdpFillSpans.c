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

static void rdpFillSpansOrg(DrawablePtr pDrawable, GCPtr pGC, int nInit, DDXPointPtr pptInit, int *pwidthInit, int fSorted)
{
	rdpGCPtr priv;
	GCFuncs *oldFuncs;

	GC_OP_PROLOGUE(pGC);
	pGC->ops->FillSpans(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);
	GC_OP_EPILOGUE(pGC);
}

void rdpFillSpans(DrawablePtr pDrawable, GCPtr pGC, int nInit, DDXPointPtr pptInit, int *pwidthInit, int fSorted)
{
	int post_process;
	RegionRec clip_reg;
	int cd;
	WindowPtr pDstWnd;
	PixmapPtr pDstPixmap;
	rdpPixmapRec *pDstPriv;

	LLOGLN(0, ("rdpFillSpans: todo"));

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
