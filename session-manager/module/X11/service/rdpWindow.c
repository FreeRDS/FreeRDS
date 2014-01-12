/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
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

#include "rdp.h"

#include "rdpWindow.h"

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
		do { if (_level < LOG_LEVEL) { ErrorF _args ; } } while (0)
#define LLOGLN(_level, _args) \
		do { if (_level < LOG_LEVEL) { ErrorF _args ; ErrorF("\n"); } } while (0)

extern rdpScreenInfoRec g_rdpScreen;
extern DevPrivateKeyRec g_rdpWindowIndex;
extern ScreenPtr g_pScreen;

/* Window Procedures */

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

	return rv;
}

int rdpValidateTree(WindowPtr pParent, WindowPtr pChild, VTKind kind)
{
	return 0;
}

void rdpPostValidateTree(WindowPtr pParent, WindowPtr pChild, VTKind kind)
{

}

void rdpWindowExposures(WindowPtr pWindow, RegionPtr prgn, RegionPtr other_exposed)
{
	ScreenPtr pScreen;
	//rdpWindowRec *priv;

	LLOGLN(10, ("rdpWindowExposures:"));
	//priv = GETWINPRIV(pWindow);
	pScreen = pWindow->drawable.pScreen;
	pScreen->WindowExposures = g_rdpScreen.WindowExposures;
	pScreen->WindowExposures(pWindow, prgn, other_exposed);

	pScreen->WindowExposures = rdpWindowExposures;
}

void rdpCopyWindow(WindowPtr pWindow, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
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
	RDS_MSG_SCREEN_BLT msg;

	LLOGLN(10, ("in rdpCopyWindow"));
	RegionInit(&reg, NullBox, 0);
	RegionCopy(&reg, prgnSrc);
	g_pScreen->CopyWindow = g_rdpScreen.CopyWindow;
	g_pScreen->CopyWindow(pWindow, ptOldOrg, prgnSrc);
	RegionInit(&clip, NullBox, 0);
	RegionCopy(&clip, &pWindow->borderClip);
	dx = pWindow->drawable.x - ptOldOrg.x;
	dy = pWindow->drawable.y - ptOldOrg.y;

	num_clip_rects = REGION_NUM_RECTS(&clip);
	num_reg_rects = REGION_NUM_RECTS(&reg);

	/* should maybe sort the rects instead of checking dy < 0 */
	/* If we can depend on the rects going from top to bottom, left to right we are ok */
	if (dy < 0 || (dy == 0 && dx < 0))
	{
		for (j = 0; j < num_clip_rects; j++)
		{
			box1 = REGION_RECTS(&clip)[j];
			rdp_set_clip(box1.x1, box1.y1, box1.x2 - box1.x1, box1.y2 - box1.y1);

			for (i = 0; i < num_reg_rects; i++)
			{
				box2 = REGION_RECTS(&reg)[i];

				msg.type = RDS_SERVER_SCREEN_BLT;
				msg.nLeftRect = box2.x1 + dx;
				msg.nTopRect = box2.y1 + dy;
				msg.nWidth = box2.x2 - box2.x1;
				msg.nHeight = box2.y2 - box2.y1;
				msg.nXSrc = box2.x1;
				msg.nYSrc = box2.y1;

				rdp_send_update((RDS_MSG_COMMON*) &msg);
			}
		}
	}
	else
	{
		for (j = num_clip_rects - 1; j >= 0; j--)
		{
			box1 = REGION_RECTS(&clip)[j];
			rdp_set_clip(box1.x1, box1.y1, box1.x2 - box1.x1, box1.y2 - box1.y1);

			for (i = num_reg_rects - 1; i >= 0; i--)
			{
				box2 = REGION_RECTS(&reg)[i];

				msg.type = RDS_SERVER_SCREEN_BLT;
				msg.nLeftRect = box2.x1 + dx;
				msg.nTopRect = box2.y1 + dy;
				msg.nWidth = box2.x2 - box2.x1;
				msg.nHeight = box2.y2 - box2.y1;
				msg.nXSrc = box2.x1;
				msg.nYSrc = box2.y1;

				rdp_send_update((RDS_MSG_COMMON*) &msg);
			}
		}
	}

	rdp_reset_clip();

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

		for (j = REGION_NUM_RECTS(&reg) - 1; j >= 0; j--)
		{
			box = REGION_RECTS(&reg)[j];
			rdp_send_area_update(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
		}

		RegionUninit(&reg);
	}

	g_pScreen->ClearToBackground = rdpClearToBackground;
}

void rdpClipNotify(WindowPtr pWindow, int dx, int dy)
{

}

void rdpRestackWindow(WindowPtr pWindow, WindowPtr pOldNextSib)
{

}

