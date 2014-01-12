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

#include "rdpPalette.h"

ColormapPtr g_rdpInstalledColormap;

/* Colormap procedures */

Bool rdpCreateColormap(ColormapPtr pColormap)
{
	return TRUE;
}

void rdpDestroyColormap(ColormapPtr pColormap)
{

}

void rdpInstallColormap(ColormapPtr pColormap)
{
	ColormapPtr oldpmap;

	oldpmap = g_rdpInstalledColormap;

	if (pColormap != oldpmap)
	{
		if (oldpmap != (ColormapPtr) None)
		{
			WalkTree(pColormap->pScreen, TellLostMap, (char*) &oldpmap->mid);
		}

		/* Install pColormap */
		g_rdpInstalledColormap = pColormap;
		WalkTree(pColormap->pScreen, TellGainedMap, (char*) &pColormap->mid);
		/*rfbSetClientColourMaps(0, 0);*/
	}

	/*g_rdpScreen.InstallColormap(pColormap);*/
}

void rdpUninstallColormap(ColormapPtr pColormap)
{
	ColormapPtr curpmap;

	curpmap = g_rdpInstalledColormap;

	if (pColormap == curpmap)
	{
		if (pColormap->mid != pColormap->pScreen->defColormap)
		{
			//curpmap = (ColormapPtr)LookupIDByType(pColormap->pScreen->defColormap, RT_COLORMAP);
			//pColormap->pScreen->InstallColormap(curpmap);
		}
	}
}

int rdpListInstalledColormaps(ScreenPtr pScreen, XID* pmaps)
{
	*pmaps = g_rdpInstalledColormap->mid;
	return 1;
}

void rdpStoreColors(ColormapPtr pColormap, int ndef, xColorItem* pdef)
{

}

void rdpResolveColor(unsigned short* pred, unsigned short* pgreen, unsigned short* pblue, VisualPtr pVisual)
{

}
