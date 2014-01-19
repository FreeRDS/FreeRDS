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
#include "rdpRandr.h"
#include "rdpScreen.h"

#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include <winpr/crt.h>
#include <winpr/stream.h>

#define LOG_LEVEL 10
#define LLOGLN(_level, _args) \
		do { if (_level < LOG_LEVEL) { ErrorF _args ; ErrorF("\n"); } } while (0)

extern rdpScreenInfoRec g_rdpScreen;

#define DEFINE_SCREEN_SIZE(_width, _height) ((_width << 16) | _height)

#define SCREEN_SIZE_WIDTH(_size) ((_size >> 16) & 0xFFFF)
#define SCREEN_SIZE_HEIGHT(_size) ((_size) & 0xFFFF)

#if 0
#define MAX_SCREEN_SIZE_WIDTH	0xFFFF
#define MAX_SCREEN_SIZE_HEIGHT	0xFFFF
#else
#define MAX_SCREEN_SIZE_WIDTH	1920
#define MAX_SCREEN_SIZE_HEIGHT	1080
#endif

static UINT32 g_StandardSizes[] =
{
	DEFINE_SCREEN_SIZE(8192, 4608),
	DEFINE_SCREEN_SIZE(4096, 2560),
	DEFINE_SCREEN_SIZE(4096, 2160),
	DEFINE_SCREEN_SIZE(3840, 2160),
	DEFINE_SCREEN_SIZE(2560, 1600), /* 16:10 */
	DEFINE_SCREEN_SIZE(2560, 1440),
	DEFINE_SCREEN_SIZE(2048, 1152),
	DEFINE_SCREEN_SIZE(2048, 1080),
	DEFINE_SCREEN_SIZE(1920, 1200), /* 16:10 */
	DEFINE_SCREEN_SIZE(1920, 1080),
	DEFINE_SCREEN_SIZE(1680, 1050), /* 16:10 */
	DEFINE_SCREEN_SIZE(1600, 1200),
	DEFINE_SCREEN_SIZE(1600, 900),
	DEFINE_SCREEN_SIZE(1440, 900), /* 16:10 */
	DEFINE_SCREEN_SIZE(1400, 1050),
	DEFINE_SCREEN_SIZE(1366, 768),
	DEFINE_SCREEN_SIZE(1280, 1024),
	DEFINE_SCREEN_SIZE(1280, 960),
	DEFINE_SCREEN_SIZE(1280, 800), /* 16:10 */
	DEFINE_SCREEN_SIZE(1280, 720),
	DEFINE_SCREEN_SIZE(1152, 864),
	DEFINE_SCREEN_SIZE(1024, 768),
	DEFINE_SCREEN_SIZE(800, 600),
	DEFINE_SCREEN_SIZE(640, 480)
};

struct _EDID
{
	BYTE Header[8];
	UINT16 ManufacturerId;
	UINT16 ManufacturerProductCode;
	UINT32 ManufacturerSerialNumber;
	BYTE WeekOfManufacture;
	BYTE YearOfManufacture;
	BYTE EdidVersion;
	BYTE EdidRevision;
	BYTE DisplayParameters[4];
	BYTE ChromacityCoordinates[10];
	BYTE BitmapTiming[4];
	BYTE StandardTiming[16];
	BYTE Descriptor1[18];
	BYTE Descriptor2[18];
	BYTE Descriptor3[18];
	BYTE Descriptor4[18];
	BYTE NumberOfExtensions;
	BYTE Checksum;
};
typedef struct _EDID EDID;

EDID* rdpConstructScreenEdid(ScreenPtr pScreen)
{
	EDID* edid;

	edid = (EDID*) malloc(sizeof(EDID));

	if (!edid)
		return NULL;

	/**
	 * 00 ff ff ff ff ff ff 00 Header
	 * 1e 6d ManufacturerId
	 * 8d 57 ManufaturerProductCode
	 * 36 21 01 00 ManufacturerSerialNumber
	 * 0a WeekOfManufacture
	 * 14 YearOfManufacture
	 * 01 EdidVersion
	 * 03 EdidRevision
	 * e0 30 1b 78 DisplayParameters
	 * ea 33 37 a5 55 4d 9d 25 11 50 ChromacityCoordinates
	 * 52 a5 4b 00 BitmapTiming
	 * b3 00 81 80 81 8f 71 4f 01 01 01 01 01 01 01 01 StandardTiming
	 * 02 3a 80 18 71 38 2d 40 58 2c 45 00 dd 0c 11 00 00 1a Descriptor1
	 * 00 00 00 fd 00 38 4b 1e 53 0f 00 0a 20 20 20 20 20 20 Descriptor2
	 * 00 00 00 fc 00 45 32 32 35 30 0a 20 20 20 20 20 20 20 Descriptor3
	 * 00 00 00 ff 00 30 31 30 4e 44 52 46 32 36 30 33 38 0a Descriptor4
	 * 00 NumberOfExtensions
	 * a2 Checksum
	 */

	ZeroMemory(edid, sizeof(EDID));

	edid->Header[0] = 0x00;
	edid->Header[1] = 0xFF;
	edid->Header[2] = 0xFF;
	edid->Header[3] = 0xFF;
	edid->Header[4] = 0xFF;
	edid->Header[5] = 0xFF;
	edid->Header[6] = 0xFF;
	edid->Header[7] = 0x00;

	edid->ManufacturerId = 0x6D1E; /* 1e 6d */
	edid->ManufacturerProductCode = 0x578D; /* 8d 57 */
	edid->ManufacturerSerialNumber = 0x21360001; /* 36 21 01 00 */

	edid->WeekOfManufacture = 10; /* 0x0A */
	edid->YearOfManufacture = 20; /* 0x14 */

	edid->EdidVersion = 1;
	edid->EdidRevision = 3;

	/* e0 30 1b 78 */
	edid->DisplayParameters[0] = 0xE0;
	edid->DisplayParameters[1] = 0x30;
	edid->DisplayParameters[2] = 0x1B;
	edid->DisplayParameters[3] = 0x78;

	/* ea 33 37 a5 55 4d 9d 25 11 50 */
	edid->ChromacityCoordinates[0] = 0xEA;
	edid->ChromacityCoordinates[1] = 0x33;
	edid->ChromacityCoordinates[2] = 0x37;
	edid->ChromacityCoordinates[3] = 0xA5;
	edid->ChromacityCoordinates[4] = 0x55;
	edid->ChromacityCoordinates[5] = 0x4D;
	edid->ChromacityCoordinates[6] = 0x9D;
	edid->ChromacityCoordinates[7] = 0x25;
	edid->ChromacityCoordinates[8] = 0x11;
	edid->ChromacityCoordinates[9] = 0x50;

	/* 52 a5 4b 00 */
	edid->BitmapTiming[0] = 0x52;
	edid->BitmapTiming[1] = 0xA5;
	edid->BitmapTiming[2] = 0x4B;
	edid->BitmapTiming[3] = 0x00;

	/* b3 00 81 80 81 8f 71 4f 01 01 01 01 01 01 01 01 */
	edid->StandardTiming[0] = 0xB3;
	edid->StandardTiming[1] = 0x00;
	edid->StandardTiming[2] = 0x81;
	edid->StandardTiming[3] = 0x80;
	edid->StandardTiming[4] = 0x81;
	edid->StandardTiming[5] = 0x8F;
	edid->StandardTiming[6] = 0x71;
	edid->StandardTiming[7] = 0x4F;
	edid->StandardTiming[8] = 0x01;
	edid->StandardTiming[9] = 0x01;
	edid->StandardTiming[10] = 0x01;
	edid->StandardTiming[11] = 0x01;
	edid->StandardTiming[12] = 0x01;
	edid->StandardTiming[13] = 0x01;
	edid->StandardTiming[14] = 0x01;
	edid->StandardTiming[15] = 0x01;

	/* 02 3a 80 18 71 38 2d 40 58 2c 45 00 dd 0c 11 00 00 1a */
	edid->Descriptor1[0] = 0x02;
	edid->Descriptor1[1] = 0x3A;
	edid->Descriptor1[2] = 0x80;
	edid->Descriptor1[3] = 0x18;
	edid->Descriptor1[4] = 0x71;
	edid->Descriptor1[5] = 0x38;
	edid->Descriptor1[6] = 0x2D;
	edid->Descriptor1[7] = 0x40;
	edid->Descriptor1[8] = 0x58;
	edid->Descriptor1[9] = 0x2C;
	edid->Descriptor1[10] = 0x45;
	edid->Descriptor1[11] = 0x00;
	edid->Descriptor1[12] = 0xDD;
	edid->Descriptor1[13] = 0x0C;
	edid->Descriptor1[14] = 0x11;
	edid->Descriptor1[15] = 0x00;
	edid->Descriptor1[16] = 0x00;
	edid->Descriptor1[17] = 0x1A;

	/* 00 00 00 fd 00 38 4b 1e 53 0f 00 0a 20 20 20 20 20 20 */
	edid->Descriptor2[0] = 0x00;
	edid->Descriptor2[1] = 0x00;
	edid->Descriptor2[2] = 0x00;
	edid->Descriptor2[3] = 0xFD;
	edid->Descriptor2[4] = 0x00;
	edid->Descriptor2[5] = 0x38;
	edid->Descriptor2[6] = 0x4B;
	edid->Descriptor2[7] = 0x1E;
	edid->Descriptor2[8] = 0x53;
	edid->Descriptor2[9] = 0x0F;
	edid->Descriptor2[10] = 0x00;
	edid->Descriptor2[11] = 0x0A;
	edid->Descriptor2[12] = 0x20;
	edid->Descriptor2[13] = 0x20;
	edid->Descriptor2[14] = 0x20;
	edid->Descriptor2[15] = 0x20;
	edid->Descriptor2[16] = 0x20;
	edid->Descriptor2[17] = 0x20;

	/* 00 00 00 fc 00 45 32 32 35 30 0a 20 20 20 20 20 20 20 */
	edid->Descriptor3[0] = 0x00;
	edid->Descriptor3[1] = 0x00;
	edid->Descriptor3[2] = 0x00;
	edid->Descriptor3[3] = 0xFC;
	edid->Descriptor3[4] = 0x00;
	edid->Descriptor3[5] = 0x45;
	edid->Descriptor3[6] = 0x32;
	edid->Descriptor3[7] = 0x32;
	edid->Descriptor3[8] = 0x35;
	edid->Descriptor3[9] = 0x30;
	edid->Descriptor3[10] = 0x0A;
	edid->Descriptor3[11] = 0x20;
	edid->Descriptor3[12] = 0x20;
	edid->Descriptor3[13] = 0x20;
	edid->Descriptor3[14] = 0x20;
	edid->Descriptor3[15] = 0x20;
	edid->Descriptor3[16] = 0x20;
	edid->Descriptor3[17] = 0x20;

	/* 00 00 00 ff 00 30 31 30 4e 44 52 46 32 36 30 33 38 0a */
	edid->Descriptor4[0] = 0x00;
	edid->Descriptor4[1] = 0x00;
	edid->Descriptor4[2] = 0x00;
	edid->Descriptor4[3] = 0xFF;
	edid->Descriptor4[4] = 0x00;
	edid->Descriptor4[5] = 0x30;
	edid->Descriptor4[6] = 0x31;
	edid->Descriptor4[7] = 0x30;
	edid->Descriptor4[8] = 0x4E;
	edid->Descriptor4[9] = 0x44;
	edid->Descriptor4[10] = 0x52;
	edid->Descriptor4[11] = 0x46;
	edid->Descriptor4[12] = 0x32;
	edid->Descriptor4[13] = 0x36;
	edid->Descriptor4[14] = 0x30;
	edid->Descriptor4[15] = 0x33;
	edid->Descriptor4[16] = 0x38;
	edid->Descriptor4[17] = 0x0A;

	edid->NumberOfExtensions = 0;

	edid->Checksum = 0xA2;

	return edid;
}

BYTE* rdpEdidToBuffer(EDID* edid)
{
	int i;
	wStream* s;
	BYTE* data;
	int length = 128;

	data = (BYTE*) malloc(length);
	ZeroMemory(data, length);

	s = Stream_New(data, length);

	Stream_Write(s, &(edid->Header), 8);
	Stream_Write_UINT16(s, edid->ManufacturerId);
	Stream_Write_UINT16(s, edid->ManufacturerProductCode);
	Stream_Write_UINT16(s, edid->ManufacturerSerialNumber);
	Stream_Write_UINT8(s, edid->WeekOfManufacture);
	Stream_Write_UINT8(s, edid->YearOfManufacture);
	Stream_Write_UINT8(s, edid->EdidVersion);
	Stream_Write_UINT8(s, edid->EdidRevision);
	Stream_Write(s, &(edid->DisplayParameters), 4);
	Stream_Write(s, &(edid->ChromacityCoordinates), 10);
	Stream_Write(s, &(edid->BitmapTiming), 4);
	Stream_Write(s, &(edid->StandardTiming), 16);
	Stream_Write(s, &(edid->Descriptor1), 18);
	Stream_Write(s, &(edid->Descriptor2), 18);
	Stream_Write(s, &(edid->Descriptor3), 18);
	Stream_Write(s, &(edid->Descriptor4), 18);
	Stream_Write_UINT8(s, edid->NumberOfExtensions);

	edid->Checksum = 0;
	Stream_Write_UINT8(s, edid->Checksum);

	for (i = 0; i < length; i++)
		edid->Checksum = (edid->Checksum + data[i]) % 256;

	edid->Checksum = 256 - edid->Checksum;

	Stream_Rewind(s, 1);
	Stream_Write_UINT8(s, edid->Checksum);

	Stream_Free(s, FALSE);

	return data;
}

static EDID* g_EDID = NULL;
static Atom edid_atom = 0;
#define EDID_ATOM_NAME		"EDID"

void rdpSetOutputEdid(RROutputPtr output, EDID* edid)
{
	BYTE* buffer;
	int length = 128;

	buffer = rdpEdidToBuffer(edid);

	if (!edid_atom)
	{
		edid_atom = MakeAtom(EDID_ATOM_NAME, sizeof(EDID_ATOM_NAME) - 1, TRUE);
	}

	if (length)
	{
		RRChangeOutputProperty(output, edid_atom, XA_INTEGER, 8,
				PropModeReplace, length, buffer, FALSE, TRUE);
	}
	else
	{
		RRDeleteOutputProperty(output, edid_atom);
	}

	free(buffer);
}

Bool rdpRRRegisterSize(ScreenPtr pScreen, int width, int height)
{
	int k;
	int index;
	int cIndex;
	int shmmax;
	int cWidth, cHeight;
	int mmWidth, mmHeight;
	RRScreenSizePtr pSizes[32];

	LLOGLN(0, ("rdpRRRegisterSize width: %d height: %d", width, height));

	index = 0;
	cIndex = 0;
	cWidth = width;
	cHeight = height;

	shmmax = get_max_shared_memory_segment_size();

	for (k = 1; k < sizeof(g_StandardSizes) / sizeof(UINT32); k++)
	{
		width = SCREEN_SIZE_WIDTH(g_StandardSizes[k]);
		height = SCREEN_SIZE_HEIGHT(g_StandardSizes[k]);

		if ((width > MAX_SCREEN_SIZE_WIDTH) || (height > MAX_SCREEN_SIZE_HEIGHT))
			continue; /* screen size is too large */

		if (shmmax > 0)
		{
			if ((width * height * 4) > shmmax)
				continue; /* required buffer size is too large */
		}

		//if (((width % 4) != 0) || ((height % 4) != 0))
		//	continue; /* disable resolutions unaligned to 4 bytes for now */

		mmWidth = rdpScreenPixelToMM(width);
		mmHeight = rdpScreenPixelToMM(height);

		if ((width == cWidth) && (height == cHeight))
			continue;

		pSizes[index] = RRRegisterSize(pScreen, width, height, mmWidth, mmHeight);
		RRRegisterRate(pScreen, pSizes[index], 60);

		index++;
	}

	width = cWidth;
	height = cHeight;

	if (cIndex < 0)
	{
		cIndex = index;

		mmWidth = rdpScreenPixelToMM(width);
		mmHeight = rdpScreenPixelToMM(height);

		pSizes[index] = RRRegisterSize(pScreen, width, height, mmWidth, mmHeight);
		RRRegisterRate(pScreen, pSizes[index], 60);
	}

	RRSetCurrentConfig(pScreen, RR_Rotate_0, 60, pSizes[cIndex]);

	return TRUE;
}

Bool rdpRRSetConfig(ScreenPtr pScreen, Rotation rotateKind, int rate, RRScreenSizePtr pSize)
{
	LLOGLN(0, ("rdpRRSetConfig: rate: %d id: %d width: %d height: %d mmWidth: %d mmHeight: %d",
			rate, pSize->id, pSize->width, pSize->height, pSize->mmWidth, pSize->mmHeight));

	return TRUE;
}

Bool rdpRRGetInfo(ScreenPtr pScreen, Rotation* pRotations)
{
	int width;
	int height;
	RRModePtr mode;
	rrScrPrivPtr pScrPriv;

	LLOGLN(0, ("rdpRRGetInfo"));

	pScrPriv = rrGetScrPriv(pScreen);

	if (pRotations)
		*pRotations = RR_Rotate_0;

	width = pScreen->width;
	height = pScreen->height;

	if (pScrPriv)
	{
		if (pScrPriv->numCrtcs > 0)
		{
			mode = pScrPriv->crtcs[0]->mode;

			if (mode)
			{
				width = mode->mode.width;
				height = mode->mode.height;
			}
		}
	}

	rdpRRRegisterSize(pScreen, width, height);

	return TRUE;
}

Bool rdpRRSetInfo(ScreenPtr pScreen)
{
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
	LLOGLN(0, ("rdpRRCrtcSet: x: %d y: %d numOutputs: %d",
			x, y, numOutputs));

	if (crtc)
	{
		crtc->x = y;
		crtc->y = y;

		if (mode && crtc->mode)
		{
			crtc->mode->mode.width = mode->mode.width;
			crtc->mode->mode.height = mode->mode.height;
		}
	}

	return RRCrtcNotify(crtc, mode, x, y, rotation, NULL, numOutputs, outputs);
}

Bool rdpRRCrtcSetGamma(ScreenPtr pScreen, RRCrtcPtr crtc)
{
	LLOGLN(0, ("rdpRRCrtcSetGamma"));

	return TRUE;
}

Bool rdpRRCrtcGetGamma(ScreenPtr pScreen, RRCrtcPtr crtc)
{
	LLOGLN(0, ("rdpRRCrtcGetGamma"));

	crtc->gammaSize = 1;

	if (!crtc->gammaRed)
	{
		crtc->gammaRed = (CARD16*) malloc(32);
		ZeroMemory(crtc->gammaRed, 32);
	}

	if (!crtc->gammaGreen)
	{
		crtc->gammaGreen = (CARD16*) malloc(32);
		ZeroMemory(crtc->gammaGreen, 32);
	}

	if (!crtc->gammaBlue)
	{
		crtc->gammaBlue = (CARD16*) malloc(32);
		ZeroMemory(crtc->gammaBlue, 32);
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
	LLOGLN(100, ("rdpRRGetPanning"));

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
	char name[64];
	RRModePtr mode;
	RRCrtcPtr crtc;
	RROutputPtr output;
	xRRModeInfo modeInfo;
#if (RANDR_INTERFACE_VERSION >= 0x0104)
	RRProviderPtr provider;
	uint32_t capabilities;
#endif
#endif
	rrScrPrivPtr pScrPriv;

	LLOGLN(0, ("rdpRRInit"));

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

	RRScreenSetSizeRange(pScreen, 8, 8, 16384, 16384);

	/**
	 * Refer to the VESA Generalized Timing Formula (GTF): GTF_V1R1.xls
	 *
	 * Modeline "String description" Dot-Clock HDisp HSyncStart HSyncEnd HTotal VDisp VSyncStart VSyncEnd VTotal [options]
	 *
	 * # 1024x768 @ 60.00 Hz (GTF) hsync: 47.70 kHz; pclk: 64.11 MHz
	 * Modeline "1024x768_60.00"  64.11  1024 1080 1184 1344  768 769 772 795  -HSync +Vsync
	 *
	 * When most of the modeline information is set to zero, xorg-server appears to be populating it with default values
	 *
	 */

	sprintf(name, "%dx%d", pScreen->width, pScreen->height);
	ZeroMemory(&modeInfo, sizeof(xRRModeInfo));

	modeInfo.id = 0;

	modeInfo.width = pScreen->width;
	modeInfo.hSyncStart = 0;
	modeInfo.hSyncEnd = 0;
	modeInfo.hTotal = 0;

	modeInfo.height = pScreen->height;
	modeInfo.vSyncStart = 0;
	modeInfo.vSyncEnd = 0;
	modeInfo.vTotal = 0;

	/* DotClock = RefreshRate * HTotal * VTotal */
	modeInfo.dotClock = 60 * modeInfo.hTotal * modeInfo.vTotal;

	modeInfo.hSkew = 0;
	modeInfo.nameLength = strlen(name);

	mode = RRModeGet(&modeInfo, name);

	if (!mode)
		return -1;

	crtc = RRCrtcCreate(pScreen, NULL);

	if (!crtc)
		return FALSE;

	RRCrtcGammaSetSize(crtc, 32);

	output = RROutputCreate(pScreen, "RDP-0", strlen("RDP-0"), NULL);

	if (!output)
		return -1;

	if (!RROutputSetClones(output, NULL, 0))
		return -1;

	if (!RROutputSetModes(output, &mode, 1, 1))
		return -1;

	if (!RROutputSetCrtcs(output, &crtc, 1))
		return -1;

	if (!RROutputSetSubpixelOrder(output, SubPixelUnknown))
		return -1;

	if (!RROutputSetPhysicalSize(output, 521, 293))
		return -1;

	if (!RROutputSetConnection(output, RR_Connected))
		return -1;

	g_EDID = rdpConstructScreenEdid(pScreen);
	rdpSetOutputEdid(output, g_EDID);

#if (RANDR_INTERFACE_VERSION >= 0x0104)
	provider = RRProviderCreate(pScreen, "RDP", strlen("RDP"));

	capabilities = RR_Capability_None;
	capabilities |= RR_Capability_SourceOutput;
	capabilities |= RR_Capability_SinkOutput;
	capabilities |= RR_Capability_SourceOffload;
	capabilities |= RR_Capability_SinkOffload;

	RRProviderSetCapabilities(provider, capabilities);
#endif

	RRCrtcNotify(crtc, mode, 0, 0, RR_Rotate_0, NULL, 1, &output);
#endif

	return 0;
}
