/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2011-2012 Jay Sorg
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
#include "rdpModes.h"
#include "rdpRandr.h"
#include "rdpScreen.h"

#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include <winpr/crt.h>
#include <winpr/stream.h>

#define LOG_LEVEL 0
#define LLOGLN(_level, _args) \
		do { if (_level < LOG_LEVEL) { ErrorF _args ; ErrorF("\n"); } } while (0)

extern rdpScreenInfoRec g_rdpScreen;

static DevPrivateKeyRec rdpRandRKeyRec;
static DevPrivateKey rdpRandRKey;

rdpRandRInfoPtr rdpGetRandRFromScreen(ScreenPtr pScreen)
{
	return ((rdpRandRInfoPtr) dixLookupPrivate(&(pScreen->devPrivates), rdpRandRKey));
}

Bool rdpRRSetConfig(ScreenPtr pScreen, Rotation rotateKind, int rate, RRScreenSizePtr pSize)
{
	LLOGLN(0, ("rdpRRSetConfig: rate: %d id: %d width: %d height: %d mmWidth: %d mmHeight: %d",
			rate, pSize->id, pSize->width, pSize->height, pSize->mmWidth, pSize->mmHeight));

	return TRUE;
}

Bool rdpRRGetInfo(ScreenPtr pScreen, Rotation* pRotations)
{
	LLOGLN(0, ("rdpRRGetInfo"));

	return TRUE;
}

Bool rdpRRScreenSetSize(ScreenPtr pScreen, CARD16 width, CARD16 height, CARD32 mmWidth, CARD32 mmHeight)
{
	BoxRec box;
	WindowPtr pRoot;
	PixmapPtr screenPixmap;

	LLOGLN(0, ("rdpRRScreenSetSize: width: %d height: %d mmWidth: %d mmHeight: %d",
			width, height, mmWidth, mmHeight));

	if ((width < 1) || (height < 1))
		return FALSE;

	SetRootClip(pScreen, FALSE);

	pRoot = pScreen->root;

	rdp_detach_framebuffer();

	g_rdpScreen.width = width;
	g_rdpScreen.height = height;

	pScreen->x = 0;
	pScreen->y = 0;
	pScreen->width = width;
	pScreen->height = height;
	pScreen->mmWidth = mmWidth;
	pScreen->mmHeight = mmHeight;

	screenInfo.x = 0;
	screenInfo.y = 0;
	screenInfo.width = width;
	screenInfo.height = height;

	if (g_rdpScreen.pfbMemory)
		rdpScreenFrameBufferFree();

	rdpScreenFrameBufferAlloc();

	screenPixmap = pScreen->GetScreenPixmap(pScreen);

	if (screenPixmap)
	{
		pScreen->ModifyPixmapHeader(screenPixmap, width, height,
				g_rdpScreen.depth, g_rdpScreen.bitsPerPixel,
				g_rdpScreen.scanline, g_rdpScreen.pfbMemory);
	}

	box.x1 = 0;
	box.y1 = 0;
	box.x2 = width;
	box.y2 = height;

	RegionInit(&pRoot->winSize, &box, 1);
	RegionInit(&pRoot->borderSize, &box, 1);
	RegionReset(&pRoot->borderClip, &box);
	RegionBreak(&pRoot->clipList);

	pRoot->drawable.width = width;
	pRoot->drawable.height = height;

	ResizeChildrenWinSize(pRoot, 0, 0, 0, 0);

	RRGetInfo(pScreen, 1);

	SetRootClip(pScreen, TRUE);

	miPaintWindow(pRoot, &pRoot->borderClip, PW_BACKGROUND);

	RRScreenSizeNotify(pScreen);

	return TRUE;
}

Bool rdpRRCrtcSet(ScreenPtr pScreen, RRCrtcPtr crtc, RRModePtr mode,
		int x, int y, Rotation rotation, int numOutputs, RROutputPtr* outputs)
{
	rdpRandRInfoPtr randr;

	LLOGLN(0, ("rdpRRCrtcSet: x: %d y: %d numOutputs: %d",
			x, y, numOutputs));

	randr = rdpGetRandRFromScreen(pScreen);

	if (!randr)
		return FALSE;

	if (crtc)
	{
		crtc->x = y;
		crtc->y = y;

		if (mode && crtc->mode)
		{
			rdpModeSelect(pScreen, mode->mode.width, mode->mode.height);
		}
	}

	RRCrtcNotify(crtc, randr->mode, x, y, rotation, NULL, numOutputs, outputs);

	return TRUE;
}

Bool rdpRRCrtcSetGamma(ScreenPtr pScreen, RRCrtcPtr crtc)
{
	LLOGLN(0, ("rdpRRCrtcSetGamma"));

	return TRUE;
}

Bool rdpRRCrtcGetGamma(ScreenPtr pScreen, RRCrtcPtr crtc)
{
	LLOGLN(0, ("rdpRRCrtcGetGamma"));

	crtc->gammaSize = 256;

	if (!crtc->gammaRed)
	{
		crtc->gammaRed = (CARD16*) malloc(crtc->gammaSize * sizeof(CARD16));
		ZeroMemory(crtc->gammaRed, crtc->gammaSize * sizeof(CARD16));
	}

	if (!crtc->gammaGreen)
	{
		crtc->gammaGreen = (CARD16*) malloc(crtc->gammaSize * sizeof(CARD16));
		ZeroMemory(crtc->gammaGreen, crtc->gammaSize * sizeof(CARD16));
	}

	if (!crtc->gammaBlue)
	{
		crtc->gammaBlue = (CARD16*) malloc(crtc->gammaSize * sizeof(CARD16));
		ZeroMemory(crtc->gammaBlue, crtc->gammaSize * sizeof(CARD16));
	}

	return TRUE;
}

Bool rdpRROutputSetProperty(ScreenPtr pScreen, RROutputPtr output, Atom property, RRPropertyValuePtr value)
{
	LLOGLN(0, ("rdpRROutputSetProperty"));

	return TRUE;
}

Bool rdpRROutputValidateMode(ScreenPtr pScreen, RROutputPtr output, RRModePtr mode)
{
	rrScrPrivPtr pScrPriv;

	LLOGLN(0, ("rdpRROutputValidateMode"));

	pScrPriv = rrGetScrPriv(pScreen);

	if ((pScrPriv->minWidth <= mode->mode.width) && (pScrPriv->maxWidth >= mode->mode.width) &&
			(pScrPriv->minHeight <= mode->mode.height) && (pScrPriv->maxHeight >= mode->mode.height))
	{
		return TRUE;
	}

	return FALSE;
}

void rdpRRModeDestroy(ScreenPtr pScreen, RRModePtr mode)
{
	LLOGLN(0, ("rdpRRModeDestroy"));
}

Bool rdpRROutputGetProperty(ScreenPtr pScreen, RROutputPtr output, Atom property)
{
	const char* name;

	name = NameForAtom(property);

	LLOGLN(0, ("rdpRROutputGetProperty: Atom: %s", name));

	if (!name)
		return FALSE;

	if (strcmp(name, "EDID"))
	{

	}

	return TRUE;
}

Bool rdpRRGetPanning(ScreenPtr pScreen, RRCrtcPtr crtc, BoxPtr totalArea, BoxPtr trackingArea, INT16* border)
{
	rdpRandRInfoPtr randr;

	LLOGLN(100, ("rdpRRGetPanning"));

	randr = rdpGetRandRFromScreen(pScreen);

	if (!randr)
		return FALSE;

	if (crtc)
	{
		LLOGLN(100, ("rdpRRGetPanning: ctrc->id: %d", crtc->id));
	}

	if (totalArea)
	{
		totalArea->x1 = 0;
		totalArea->y1 = 0;
		totalArea->x2 = pScreen->width;
		totalArea->y2 = pScreen->height;

		LLOGLN(100, ("rdpRRGetPanning: totalArea: x1: %d y1: %d x2: %d y2: %d",
				totalArea->x1, totalArea->y1, totalArea->x2, totalArea->y2));
	}

	if (trackingArea)
	{
		trackingArea->x1 = 0;
		trackingArea->y1 = 0;
		trackingArea->x2 = pScreen->width;
		trackingArea->y2 = pScreen->height;

		LLOGLN(100, ("rdpRRGetPanning: trackingArea: x1: %d y1: %d x2: %d y2: %d",
				trackingArea->x1, trackingArea->y1, trackingArea->x2, trackingArea->y2));
	}

	if (border)
	{
		border[0] = 0;
		border[1] = 0;
		border[2] = 0;
		border[3] = 0;
	}

	return TRUE;
}

Bool rdpRRSetPanning(ScreenPtr pScrn, RRCrtcPtr crtc, BoxPtr totalArea, BoxPtr trackingArea, INT16* border)
{
	LLOGLN(0, ("rdpRRSetPanning"));

	return TRUE;
}

#if (RANDR_INTERFACE_VERSION >= 0x0104)

Bool rdpRRCrtcSetScanoutPixmap(RRCrtcPtr crtc, PixmapPtr pixmap)
{
	LLOGLN(0, ("rdpRRCrtcSetScanoutPixmap"));

	return TRUE;
}

Bool rdpRRProviderSetOutputSource(ScreenPtr pScreen, RRProviderPtr provider, RRProviderPtr output_source)
{
	LLOGLN(0, ("rdpRRProviderSetOutputSource"));

	return TRUE;
}

Bool rdpRRProviderSetOffloadSink(ScreenPtr pScreen, RRProviderPtr provider, RRProviderPtr offload_sink)
{
	LLOGLN(0, ("rdpRRProviderSetOffloadSink"));

	return TRUE;
}

Bool rdpRRProviderGetProperty(ScreenPtr pScreen, RRProviderPtr provider, Atom property)
{
	LLOGLN(0, ("rdpRRProviderGetProperty"));

	return TRUE;
}

Bool rdpRRProviderSetProperty(ScreenPtr pScreen, RRProviderPtr provider, Atom property, RRPropertyValuePtr value)
{
	LLOGLN(0, ("rdpRRProviderGetProperty"));

	return TRUE;
}

void rdpRRProviderDestroy(ScreenPtr pScreen, RRProviderPtr provider)
{
	LLOGLN(0, ("rdpRRProviderDestroy"));
}

#endif

int rdpRRInit(ScreenPtr pScreen)
{
#if RANDR_12_INTERFACE
	RRCrtcPtr crtc;
	RROutputPtr output;
	rdpRandRInfoPtr randr;
#if (RANDR_INTERFACE_VERSION >= 0x0104)
	RRProviderPtr provider;
	uint32_t capabilities;
#endif
#endif
	rrScrPrivPtr pScrPriv;

	LLOGLN(0, ("rdpRRInit"));

	rdpRandRKey = &rdpRandRKeyRec;

	if (!dixRegisterPrivateKey(&rdpRandRKeyRec, PRIVATE_SCREEN, 0))
		return FALSE;

	randr = (rdpRandRInfoPtr) malloc(sizeof(rdpRandRInfoRec));
	ZeroMemory(randr, sizeof(rdpRandRInfoRec));

	if (!RRScreenInit(pScreen))
		return -1;

	pScrPriv = rrGetScrPriv(pScreen);

	dixSetPrivate(&pScreen->devPrivates, rdpRandRKey, randr);

	pScrPriv->rrSetConfig = rdpRRSetConfig;

	pScrPriv->rrGetInfo = rdpRRGetInfo;

	pScrPriv->rrScreenSetSize = rdpRRScreenSetSize;

#if RANDR_12_INTERFACE
	pScrPriv->rrCrtcSet = rdpRRCrtcSet;
	pScrPriv->rrCrtcSetGamma = rdpRRCrtcSetGamma;
	pScrPriv->rrCrtcGetGamma = rdpRRCrtcGetGamma;
	pScrPriv->rrOutputSetProperty = rdpRROutputSetProperty;
	pScrPriv->rrOutputValidateMode = rdpRROutputValidateMode;
	pScrPriv->rrModeDestroy = rdpRRModeDestroy;

#if RANDR_13_INTERFACE
	pScrPriv->rrOutputGetProperty = rdpRROutputGetProperty;
	pScrPriv->rrGetPanning = rdpRRGetPanning;
	pScrPriv->rrSetPanning = rdpRRSetPanning;
#endif

#if (RANDR_INTERFACE_VERSION >= 0x0104)
	pScrPriv->rrCrtcSetScanoutPixmap = rdpRRCrtcSetScanoutPixmap;
	pScrPriv->rrProviderSetOutputSource = rdpRRProviderSetOutputSource;
	pScrPriv->rrProviderSetOffloadSink = rdpRRProviderSetOffloadSink;
	pScrPriv->rrProviderGetProperty = rdpRRProviderGetProperty;
	pScrPriv->rrProviderSetProperty = rdpRRProviderSetProperty;
	pScrPriv->rrProviderDestroy = rdpRRProviderDestroy;
#endif

	randr->width = pScreen->width;
	randr->height = pScreen->height;

	rdpProbeModes(pScreen);

	RRScreenSetSizeRange(pScreen, 8, 8, 16384, 16384);

	crtc = RRCrtcCreate(pScreen, NULL);

	if (!crtc)
		return FALSE;

	RRCrtcGammaSetSize(crtc, 256);

	output = RROutputCreate(pScreen, "RDP-0", strlen("RDP-0"), NULL);

	if (!output)
		return -1;

	if (!RROutputSetClones(output, NULL, 0))
		return -1;

	if (!RROutputSetModes(output, randr->modes, randr->numModes, 0))
		return -1;

	if (!RROutputSetCrtcs(output, &crtc, 1))
		return -1;

	if (!RROutputSetSubpixelOrder(output, SubPixelUnknown))
		return -1;

	if (!RROutputSetPhysicalSize(output, 521, 293))
		return -1;

	if (!RROutputSetConnection(output, RR_Connected))
		return -1;

	pScrPriv->primaryOutput = output;

	randr->edid = rdpConstructScreenEdid(pScreen);
	rdpSetOutputEdid(output, randr->edid);

#if (RANDR_INTERFACE_VERSION >= 0x0104)
	provider = RRProviderCreate(pScreen, "RDP", strlen("RDP"));

	capabilities = RR_Capability_None;
	capabilities |= RR_Capability_SourceOutput;
	capabilities |= RR_Capability_SinkOutput;
	capabilities |= RR_Capability_SourceOffload;
	capabilities |= RR_Capability_SinkOffload;

	RRProviderSetCapabilities(provider, capabilities);
#endif

	RRCrtcNotify(crtc, randr->mode, 0, 0, RR_Rotate_0, NULL, 1, &output);
#endif

	rdpWriteGnomeMonitorConfiguration(pScreen);

	return 0;
}
