/*
Copyright 2011-2012 Jay Sorg

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

RandR extension implementation

 */

#include "rdp.h"
#include "rdprandr.h"

#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>

extern rdpScreenInfoRec g_rdpScreen;
extern WindowPtr g_invalidate_window;

static XID g_wid = 0;

#define DEFINE_SCREEN_SIZE(_width, _height) ((_width << 16) | _height)

#define SCREEN_SIZE_WIDTH(_size) ((_size >> 16) & 0xFFFF)
#define SCREEN_SIZE_HEIGHT(_size) ((_size) & 0xFFFF)

static UINT32 g_StandardSizes[] =
{
	DEFINE_SCREEN_SIZE(8192, 4608),
	DEFINE_SCREEN_SIZE(4096, 2560),
	DEFINE_SCREEN_SIZE(4096, 2160),
	DEFINE_SCREEN_SIZE(3840, 2160),
	DEFINE_SCREEN_SIZE(2560, 1600),
	DEFINE_SCREEN_SIZE(2560, 1440),
	DEFINE_SCREEN_SIZE(2048, 1152),
	DEFINE_SCREEN_SIZE(2048, 1080),
	DEFINE_SCREEN_SIZE(1920, 1080),
	DEFINE_SCREEN_SIZE(1680, 1050),
	DEFINE_SCREEN_SIZE(1600, 1200),
	DEFINE_SCREEN_SIZE(1600, 900),
	DEFINE_SCREEN_SIZE(1440, 900),
	DEFINE_SCREEN_SIZE(1400, 1050),
	DEFINE_SCREEN_SIZE(1366, 768),
	DEFINE_SCREEN_SIZE(1280, 1024),
	DEFINE_SCREEN_SIZE(1280, 960),
	DEFINE_SCREEN_SIZE(1280, 800),
	DEFINE_SCREEN_SIZE(1280, 720),
	DEFINE_SCREEN_SIZE(1152, 864),
	DEFINE_SCREEN_SIZE(1024, 768),
	DEFINE_SCREEN_SIZE(800, 600),
	DEFINE_SCREEN_SIZE(640, 480)
};

Bool rdpRRRegisterSize(ScreenPtr pScreen, int width, int height)
{
	int index;
	int cIndex;
	int cWidth, cHeight;
	int mmWidth, mmHeight;
	RRScreenSizePtr pSizes[32];

	cIndex = -1;
	cWidth = width;
	cHeight = height;

	for (index = 0; index < sizeof(g_StandardSizes) / sizeof(UINT32); index++)
	{
		width = SCREEN_SIZE_WIDTH(g_StandardSizes[index]);
		height = SCREEN_SIZE_HEIGHT(g_StandardSizes[index]);

		mmWidth = PixelToMM(width);
		mmHeight = PixelToMM(height);

		if ((width == cWidth) && (height == cHeight))
			cIndex = index;

		pSizes[index] = RRRegisterSize(pScreen, width, height, mmWidth, mmHeight);
		RRRegisterRate(pScreen, pSizes[index], 60);
	}

	width = cWidth;
	height = cHeight;

	if (cIndex < 0)
	{
		cIndex = index;

		mmWidth = PixelToMM(width);
		mmHeight = PixelToMM(height);

		pSizes[index] = RRRegisterSize(pScreen, width, height, mmWidth, mmHeight);
		RRRegisterRate(pScreen, pSizes[index], 60);
	}

	RRSetCurrentConfig(pScreen, RR_Rotate_0, 60, pSizes[cIndex]);

	return TRUE;
}

Bool rdpRRSetConfig(ScreenPtr pScreen, Rotation rotateKind, int rate, RRScreenSizePtr pSize)
{
	return TRUE;
}

Bool rdpRRGetInfo(ScreenPtr pScreen, Rotation* pRotations)
{
	int width;
	int height;

	if (pRotations)
		*pRotations = RR_Rotate_0;

	width = g_rdpScreen.width;
	height = g_rdpScreen.height;

	rdpRRRegisterSize(pScreen, width, height);

	return TRUE;
}

/**
 * for lack of a better way, a window is created that covers
 * the area and when its deleted, it's invalidated
 */
static int rdpInvalidateArea(ScreenPtr pScreen, int x, int y, int width, int height)
{
	int attri;
	Mask mask;
	int result;
	WindowPtr pWin;
	XID attributes[4];

	mask = 0;
	attri = 0;
	attributes[attri++] = pScreen->blackPixel;
	mask |= CWBackPixel;
	attributes[attri++] = xTrue;
	mask |= CWOverrideRedirect;

	if (g_wid == 0)
	{
		g_wid = FakeClientID(0);
	}

	pWin = CreateWindow(g_wid, pScreen->root,
			x, y, width, height, 0, InputOutput, mask,
			attributes, 0, serverClient,
			wVisual(pScreen->root), &result);

	if (result == 0)
	{
		g_invalidate_window = pWin;
		MapWindow(pWin, serverClient);
		DeleteWindow(pWin, None);
		g_invalidate_window = pWin;
	}

	return 0;
}

Bool rdpRRScreenSetSize(ScreenPtr pScreen, CARD16 width, CARD16 height, CARD32 mmWidth, CARD32 mmHeight)
{
	BoxRec box;
	PixmapPtr screenPixmap;

	if ((width < 1) || (height < 1))
	{
		return FALSE;
	}

	rdpup_detach_framebuffer();

	g_rdpScreen.width = width;
	g_rdpScreen.height = height;
	g_rdpScreen.paddedWidthInBytes = PixmapBytePad(g_rdpScreen.width, g_rdpScreen.depth);
	g_rdpScreen.sizeInBytes = g_rdpScreen.paddedWidthInBytes * g_rdpScreen.height;

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
	{
		if (g_rdpScreen.sharedMemory)
		{
			/* detach shared memory segment */
			shmdt(g_rdpScreen.pfbMemory);
			g_rdpScreen.pfbMemory = NULL;

			/* deallocate shared memory segment */
			shmctl(g_rdpScreen.segmentId, IPC_RMID, 0);

			/* allocate shared memory segment */
			g_rdpScreen.segmentId = shmget(IPC_PRIVATE, g_rdpScreen.sizeInBytes,
					IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

			/* attach the shared memory segment */
			g_rdpScreen.pfbMemory = (char*) shmat(g_rdpScreen.segmentId, 0, 0);
		}
		else
		{
			g_rdpScreen.pfbMemory = (char*) malloc(g_rdpScreen.sizeInBytes);
		}

		if (!g_rdpScreen.pfbMemory)
		{
			rdpLog("rdpScreenInit pfbMemory malloc failed\n");
			return 0;
		}

		ZeroMemory(g_rdpScreen.pfbMemory, g_rdpScreen.sizeInBytes);
	}

	screenPixmap = pScreen->GetScreenPixmap(pScreen);

	if (screenPixmap)
	{
		pScreen->ModifyPixmapHeader(screenPixmap, width, height,
				g_rdpScreen.depth, g_rdpScreen.bitsPerPixel,
				g_rdpScreen.paddedWidthInBytes,
				g_rdpScreen.pfbMemory);
	}

	box.x1 = 0;
	box.y1 = 0;
	box.x2 = width;
	box.y2 = height;

	RegionInit(&pScreen->root->winSize, &box, 1);
	RegionInit(&pScreen->root->borderSize, &box, 1);
	RegionReset(&pScreen->root->borderClip, &box);
	RegionBreak(&pScreen->root->clipList);

	pScreen->root->drawable.width = width;
	pScreen->root->drawable.height = height;

	ResizeChildrenWinSize(pScreen->root, 0, 0, 0, 0);

	RRGetInfo(pScreen, 1);

	rdpInvalidateArea(pScreen, 0, 0, pScreen->width, pScreen->height);

	RRScreenSizeNotify(pScreen);
	RRTellChanged(pScreen);

	return TRUE;
}

Bool rdpRRCrtcSet(ScreenPtr pScreen, RRCrtcPtr crtc, RRModePtr mode,
		int x, int y, Rotation rotation, int numOutputs, RROutputPtr* outputs)
{
	return TRUE;
}

Bool rdpRRCrtcSetGamma(ScreenPtr pScreen, RRCrtcPtr crtc)
{
	return TRUE;
}

Bool rdpRRCrtcGetGamma(ScreenPtr pScreen, RRCrtcPtr crtc)
{
	return TRUE;
}

Bool rdpRROutputSetProperty(ScreenPtr pScreen, RROutputPtr output, Atom property, RRPropertyValuePtr value)
{
	return TRUE;
}

Bool rdpRROutputValidateMode(ScreenPtr pScreen, RROutputPtr output, RRModePtr mode)
{
	return TRUE;
}

void rdpRRModeDestroy(ScreenPtr pScreen, RRModePtr mode)
{

}

Bool rdpRROutputGetProperty(ScreenPtr pScreen, RROutputPtr output, Atom property)
{
	return TRUE;
}

Bool rdpRRGetPanning(ScreenPtr pScrn, RRCrtcPtr crtc, BoxPtr totalArea, BoxPtr trackingArea, INT16* border)
{
	if (totalArea != 0)
	{
		totalArea->x1 = 0;
		totalArea->y1 = 0;
		totalArea->x2 = g_rdpScreen.width;
		totalArea->y2 = g_rdpScreen.height;
	}

	if (trackingArea != 0)
	{
		trackingArea->x1 = 0;
		trackingArea->y1 = 0;
		trackingArea->x2 = g_rdpScreen.width;
		trackingArea->y2 = g_rdpScreen.height;
	}

	if (border != 0)
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
	return TRUE;
}

int rdpRRInit(ScreenPtr pScreen)
{
#if RANDR_12_INTERFACE
	char name[64];
	RRModePtr mode;
	RRCrtcPtr crtc;
	RROutputPtr output;
	xRRModeInfo modeInfo;
#endif
	rrScrPrivPtr pScrPriv;

	if (!RRScreenInit(pScreen))
		return -1;

	pScrPriv = rrGetScrPriv(pScreen);

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

	pScrPriv->rrOutputGetProperty = rdpRROutputGetProperty;
	pScrPriv->rrGetPanning = rdpRRGetPanning;
	pScrPriv->rrSetPanning = rdpRRSetPanning;

	RRScreenSetSizeRange(pScreen, 8, 8, 16384, 16384);

	sprintf(name, "%dx%d", pScreen->width, pScreen->height);
	ZeroMemory(&modeInfo, sizeof(xRRModeInfo));
	modeInfo.width = pScreen->width;
	modeInfo.height = pScreen->height;
	modeInfo.nameLength = strlen (name);

	mode = RRModeGet(&modeInfo, name);

	if (!mode)
		return -1;

	crtc = RRCrtcCreate(pScreen, NULL);

	if (!crtc)
		return FALSE;

	output = RROutputCreate(pScreen, "RDP-0", strlen("RDP-0"), NULL);

	if (!output)
		return -1;

	if (!RROutputSetClones(output, NULL, 0))
		return -1;

	if (!RROutputSetModes(output, &mode, 1, 0))
		return -1;

	if (!RROutputSetCrtcs(output, &crtc, 1))
		return -1;

	if (!RROutputSetConnection(output, RR_Connected))
		return -1;

	if (!RROutputSetSubpixelOrder(output, SubPixelHorizontalRGB))
		return -1;

	if (!RROutputSetPhysicalSize(output, 521, 293))
		return -1;

	RRCrtcNotify(crtc, mode, 0, 0, RR_Rotate_0, NULL, 1, &output);
#endif

	return 0;
}
