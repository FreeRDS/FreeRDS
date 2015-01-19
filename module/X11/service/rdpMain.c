/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2005-2012 Jay Sorg
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

#if (XORG_VERSION_CURRENT >= XORG_VERSION(1,13,0))
#include "glx_extinit.h"
#else

#include "glxserver.h"
#include "xf86Module.h"

#ifdef GLXEXT
#undef GLXEXT
#endif

#endif

#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include <winpr/crt.h>
#include <winpr/pipe.h>

#if 0
#define DEBUG_OUT(fmt, ...) ErrorF(fmt, ##__VA_ARGS__)
#else
#define DEBUG_OUT(fmt, ...)
#endif

rdpScreenInfoRec g_rdpScreen;
ScreenPtr g_pScreen = 0;

DevPrivateKeyRec g_rdpGCIndex;
DevPrivateKeyRec g_rdpWindowIndex;
DevPrivateKeyRec g_rdpPixmapIndex;

/* main mouse and keyboard devices */
DeviceIntPtr g_pointer = 0;
DeviceIntPtr g_keyboard = 0;

rdpPixmapRec g_screenPriv;

/* set all these at once, use function set_bpp */
int g_bpp = 16;
int g_Bpp = 2;
int g_Bpp_mask = 0xFFFF;
static int g_firstTime = 1;
static int g_redBits = 5;
static int g_greenBits = 6;
static int g_blueBits = 5;
static int g_initOutputCalled = 0;

/* Common pixmap formats */
static PixmapFormatRec g_formats[MAXFORMATS] =
{
	{ 1, 1, BITMAP_SCANLINE_PAD },
	{ 4, 8, BITMAP_SCANLINE_PAD },
	{ 8, 8, BITMAP_SCANLINE_PAD },
	{ 15, 16, BITMAP_SCANLINE_PAD },
	{ 16, 16, BITMAP_SCANLINE_PAD },
	{ 24, 32, BITMAP_SCANLINE_PAD },
	{ 32, 32, BITMAP_SCANLINE_PAD },
};

static int g_numFormats = 7;

static miPointerSpriteFuncRec g_rdpSpritePointerFuncs =
{
	/* these are in rdpinput.c */
	rdpSpriteRealizeCursor,
	rdpSpriteUnrealizeCursor,
	rdpSpriteSetCursor,
	rdpSpriteMoveCursor,
	rdpSpriteDeviceCursorInitialize,
	rdpSpriteDeviceCursorCleanup
};

static miPointerScreenFuncRec g_rdpPointerCursorFuncs =
{
	/* these are in rdpinput.c */
	rdpCursorOffScreen,
	rdpCrossScreen,
	rdpPointerWarpCursor
};

/* returns error, zero is good */
static int set_bpp(int bpp)
{
	int rv;

	rv = 0;
	g_bpp = bpp;

	if (g_bpp == 8)
	{
		g_Bpp = 1;
		g_Bpp_mask = 0xff;
		g_redBits = 3;
		g_greenBits = 3;
		g_blueBits = 2;
	}
	else if (g_bpp == 15)
	{
		g_Bpp = 2;
		g_Bpp_mask = 0x7FFF;
		g_redBits = 5;
		g_greenBits = 5;
		g_blueBits = 5;
	}
	else if (g_bpp == 16)
	{
		g_Bpp = 2;
		g_Bpp_mask = 0xFFFF;
		g_redBits = 5;
		g_greenBits = 6;
		g_blueBits = 5;
	}
	else if (g_bpp == 24)
	{
		g_Bpp = 4;
		g_Bpp_mask = 0xFFFFFF;
		g_redBits = 8;
		g_greenBits = 8;
		g_blueBits = 8;
	}
	else if (g_bpp == 32)
	{
		g_Bpp = 4;
		g_Bpp_mask = 0xFFFFFF;
		g_redBits = 8;
		g_greenBits = 8;
		g_blueBits = 8;
	}
	else
	{
		rv = 1;
	}

	return rv;
}

#if (XORG_VERSION_CURRENT >= XORG_VERSION(1,13,0))
Bool rdpCloseScreen(ScreenPtr pScreen)
#else
Bool rdpCloseScreen(int index, ScreenPtr pScreen)
#endif
{
	pScreen->CloseScreen = g_rdpScreen.CloseScreen;
	pScreen->CreateGC = g_rdpScreen.CreateGC;
	//pScreen->PaintWindowBackground = g_rdpScreen.PaintWindowBackground;
	//pScreen->PaintWindowBorder = g_rdpScreen.PaintWindowBorder;
	pScreen->CopyWindow = g_rdpScreen.CopyWindow;
	pScreen->ClearToBackground = g_rdpScreen.ClearToBackground;
	//pScreen->RestoreAreas = g_rdpScreen.RestoreAreas;
	return TRUE;
}

void rdpQueryBestSize(int xclass, unsigned short* pWidth, unsigned short* pHeight, ScreenPtr pScreen)
{
	unsigned short w;

	switch (xclass)
	{
		case CursorShape:

			*pWidth = 96;
			*pHeight = 96;

			if (*pWidth > pScreen->width)
				*pWidth = pScreen->width;
			if (*pHeight > pScreen->height)
				*pHeight = pScreen->height;

			break;

		case TileShape:
		case StippleShape:

			w = *pWidth;

			if ((w & (w - 1)) && w < FB_UNIT)
			{
				for (w = 1; w < *pWidth; w <<= 1);
					*pWidth = w;
			}
	}
}

Bool rdpSaveScreen(ScreenPtr pScreen, int on)
{
	return TRUE;
}

#if (XORG_VERSION_CURRENT >= XORG_VERSION(1,13,0))
static void rdpWakeupHandler(ScreenPtr pScreen, unsigned long result, pointer pReadmask)
{
	pScreen->WakeupHandler = g_rdpScreen.WakeupHandler;
	pScreen->WakeupHandler(pScreen, result, pReadmask);
	pScreen->WakeupHandler = rdpWakeupHandler;
}
#else
static void rdpWakeupHandler(int screenNum, pointer wakeupData, unsigned long result, pointer pReadmask)
{
	ScreenPtr pScreen = g_pScreen;
	pScreen->WakeupHandler = g_rdpScreen.WakeupHandler;
	pScreen->WakeupHandler(screenNum, wakeupData, result, pReadmask);
	pScreen->WakeupHandler = rdpWakeupHandler;
}
#endif

static void rdpBlockHandler1(pointer blockData, OSTimePtr pTimeout, pointer pReadmask)
{

}

static void rdpWakeupHandler1(pointer blockData, int result, pointer pReadmask)
{
	rdp_check();
}

/* device cursor procedures */

Bool rdpDeviceCursorInitialize(DeviceIntPtr pDev, ScreenPtr pScreen)
{
	DEBUG_OUT("rdpDeviceCursorInitializeProcPtr:\n");
	return TRUE;
}

void rdpDeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScreen)
{
	DEBUG_OUT("rdpDeviceCursorCleanupProcPtr:\n");
}

void rdpClientStateChange(CallbackListPtr* cbl, pointer myData, pointer clt)
{
	dispatchException &= ~DE_RESET; /* hack - force server not to reset */
}

#if (XORG_VERSION_CURRENT >= XORG_VERSION(1,13,0))
Bool rdpScreenInit(ScreenPtr pScreen, int argc, char** argv)
#else
Bool rdpScreenInit(int index, ScreenPtr pScreen, int argc, char** argv)
#endif
{
	int dpix;
	int dpiy;
	int ret;
	Bool vis_found;
	VisualPtr vis;
	PictureScreenPtr ps;

	g_pScreen = pScreen;
	ZeroMemory(&g_screenPriv, sizeof(g_screenPriv));

	g_rdpScreen.dpi = 96;
	dpix = g_rdpScreen.dpi;
	dpiy = g_rdpScreen.dpi;
	monitorResolution = g_rdpScreen.dpi;

	ErrorF("X11rdp, an X11 server for FreeRDS\n");
	ErrorF("Version %s\n", X11RDPVER);
	ErrorF("Copyright (C) 2005-2012 Jay Sorg\n");
	ErrorF("Copyright (C) 2013 Thincast Technologies GmbH\n");

	rdpScreenFrameBufferAlloc();

	miClearVisualTypes();

	if (defaultColorVisualClass == -1)
	{
		defaultColorVisualClass = TrueColor;
	}

	if (!miSetVisualTypes(g_rdpScreen.depth,
			miGetDefaultVisualMask(g_rdpScreen.depth),
			8, defaultColorVisualClass))
	{
		ErrorF("rdpScreenInit miSetVisualTypes failed\n");
		return 0;
	}

	miSetPixmapDepths();

	ret = fbScreenInit(pScreen, g_rdpScreen.pfbMemory,
			g_rdpScreen.width, g_rdpScreen.height,
			dpix, dpiy, g_rdpScreen.scanline / g_rdpScreen.bytesPerPixel, g_rdpScreen.bitsPerPixel);

	if (!ret)
	{
		DEBUG_OUT("rdpScreenInit: error\n");
		return 0;
	}

	/* this is for rgb, not bgr, just doing rgb for now */
	vis = pScreen->visuals + (pScreen->numVisuals - 1);

	while (vis >= pScreen->visuals)
	{
		if ((vis->class | DynamicClass) == DirectColor)
		{
			vis->offsetBlue = 0;
			vis->blueMask = (1 << g_blueBits) - 1;
			vis->offsetGreen = g_blueBits;
			vis->greenMask = ((1 << g_greenBits) - 1) << vis->offsetGreen;
			vis->offsetRed = g_blueBits + g_greenBits;
			vis->redMask = ((1 << g_redBits) - 1) << vis->offsetRed;
		}

		vis--;
	}

	if (g_rdpScreen.bitsPerPixel > 4)
	{
		fbPictureInit(pScreen, 0, 0);
	}

	if (!dixRegisterPrivateKey(&g_rdpGCIndex, PRIVATE_GC, sizeof(rdpGCRec)))
	{
		FatalError("rdpScreenInit: dixRegisterPrivateKey PRIVATE_GC failed\n");
	}

	if (!dixRegisterPrivateKey(&g_rdpWindowIndex, PRIVATE_WINDOW, sizeof(rdpWindowRec)))
	{
		FatalError("rdpScreenInit: dixRegisterPrivateKey PRIVATE_WINDOW failed\n");
	}

	if (!dixRegisterPrivateKey(&g_rdpPixmapIndex, PRIVATE_PIXMAP, sizeof(rdpPixmapRec)))
	{
		FatalError("rdpScreenInit: dixRegisterPrivateKey PRIVATE_PIXMAP failed\n");
	}

	/* Random screen procedures */
	g_rdpScreen.CloseScreen = pScreen->CloseScreen;
	pScreen->CloseScreen = rdpCloseScreen;
	g_rdpScreen.QueryBestSize = pScreen->QueryBestSize;
	pScreen->QueryBestSize = rdpQueryBestSize;
	g_rdpScreen.SaveScreen = pScreen->SaveScreen;
	pScreen->SaveScreen = rdpSaveScreen;
	//g_rdpScreen.GetImage = pScreen->GetImage;
	//pScreen->GetImage = rdpGetImage;
	//g_rdpScreen.GetSpans = pScreen->GetSpans;
	//pScreen->GetSpans = rdpGetSpans;
	//g_rdpScreen.SourceValidate = pScreen->SourceValidate;
	//pScreen->SourceValidate = rdpSourceValidate;

	/* GC procedures */
	g_rdpScreen.CreateGC = pScreen->CreateGC;
	pScreen->CreateGC = rdpCreateGC;

	/* Pixmap procedures */
	g_rdpScreen.CreatePixmap = pScreen->CreatePixmap;
	g_rdpScreen.DestroyPixmap = pScreen->DestroyPixmap;
	pScreen->CreatePixmap = rdpCreatePixmap;
	pScreen->DestroyPixmap = rdpDestroyPixmap;

	/* Window Procedures */
	g_rdpScreen.CreateWindow = pScreen->CreateWindow;
	g_rdpScreen.DestroyWindow = pScreen->DestroyWindow;
	g_rdpScreen.ChangeWindowAttributes = pScreen->ChangeWindowAttributes;
	g_rdpScreen.RealizeWindow = pScreen->RealizeWindow;
	g_rdpScreen.UnrealizeWindow = pScreen->UnrealizeWindow;
	g_rdpScreen.PositionWindow = pScreen->PositionWindow;
	g_rdpScreen.WindowExposures = pScreen->WindowExposures;
	g_rdpScreen.CopyWindow = pScreen->CopyWindow;
	g_rdpScreen.ClearToBackground = pScreen->ClearToBackground;
	pScreen->CreateWindow = rdpCreateWindow;
	pScreen->DestroyWindow = rdpDestroyWindow;
	pScreen->ChangeWindowAttributes = rdpChangeWindowAttributes;
	pScreen->RealizeWindow = rdpRealizeWindow;
	pScreen->UnrealizeWindow = rdpUnrealizeWindow;
	pScreen->PositionWindow = rdpPositionWindow;
	pScreen->WindowExposures = rdpWindowExposures;
	pScreen->CopyWindow = rdpCopyWindow;
	pScreen->ClearToBackground = rdpClearToBackground;

	/* Backing store procedures */
	//g_rdpScreen.RestoreAreas = pScreen->RestoreAreas;
	//pScreen->RestoreAreas = rdpRestoreAreas;

	/* os layer procedures */
	g_rdpScreen.WakeupHandler = pScreen->WakeupHandler;
	pScreen->WakeupHandler = rdpWakeupHandler;
	//g_rdpScreen.BlockHandler = pScreen->BlockHandler;
	//pScreen->BlockHandler = rdpBlockHandler;

	/* Colormap procedures */
	g_rdpScreen.CreateColormap = pScreen->CreateColormap;
	g_rdpScreen.DestroyColormap = pScreen->DestroyColormap;
	g_rdpScreen.InstallColormap = pScreen->InstallColormap;
	g_rdpScreen.UninstallColormap = pScreen->UninstallColormap;
	g_rdpScreen.ListInstalledColormaps = pScreen->ListInstalledColormaps;
	g_rdpScreen.StoreColors = pScreen->StoreColors;
	g_rdpScreen.ResolveColor = pScreen->ResolveColor;
	pScreen->CreateColormap = rdpCreateColormap;
	pScreen->DestroyColormap = rdpDestroyColormap;
	pScreen->InstallColormap = rdpInstallColormap;
	pScreen->UninstallColormap = rdpUninstallColormap;
	pScreen->ListInstalledColormaps = rdpListInstalledColormaps;
	pScreen->StoreColors = rdpStoreColors;
	pScreen->ResolveColor = rdpResolveColor;

	ps = GetPictureScreenIfSet(pScreen);

	if (ps)
	{
		g_rdpScreen.Composite = ps->Composite;
		g_rdpScreen.Glyphs = ps->Glyphs;
	}

	pScreen->blackPixel = g_rdpScreen.blackPixel = 0;
	pScreen->whitePixel = g_rdpScreen.whitePixel = 0;

	if (ps)
	{
		ps->Composite = rdpComposite;
		ps->Glyphs = rdpGlyphs;
	}

	miPointerInitialize(pScreen, &g_rdpSpritePointerFuncs, &g_rdpPointerCursorFuncs, 1);

#if 0
	/* device cursor procedures */
	pScreen->DeviceCursorInitialize = rdpDeviceCursorInitialize;
	pScreen->DeviceCursorCleanup = rdpDeviceCursorCleanup;
#endif

	vis_found = 0;
	vis = pScreen->visuals + (pScreen->numVisuals - 1);

	while (vis >= pScreen->visuals)
	{
		if (vis->vid == pScreen->rootVisual)
		{
			vis_found = 1;
		}

		vis--;
	}

	if (!vis_found)
	{
		ErrorF("rdpScreenInit: couldn't find root visual\n");
		exit(1);
	}

	ret = 1;

	if (ret)
	{
		ret = fbCreateDefColormap(pScreen);

		if (!ret)
		{
			DEBUG_OUT("rdpScreenInit: fbCreateDefColormap failed\n");
		}
	}

	if (ret)
	{
		ret = rdp_init();

		if (!ret)
		{
			DEBUG_OUT("rdpScreenInit: rdpup_init failed\n");
		}
	}

	if (ret)
	{
		RegisterBlockAndWakeupHandlers(rdpBlockHandler1, rdpWakeupHandler1, NULL);
	}

	rdpRRInit(pScreen);

	DEBUG_OUT("rdpScreenInit: ret %d\n", ret);

	return ret;
}

/* this is the first function called, it can be called many times
   returns the number or parameters processed
   if it dosen't apply to the rdp part, return 0 */
int ddxProcessArgument(int argc, char** argv, int i)
{
	if (g_firstTime)
	{
		ZeroMemory(&g_rdpScreen, sizeof(g_rdpScreen));
		g_rdpScreen.width  = 1024;
		g_rdpScreen.height = 768;
		g_rdpScreen.depth = 24;
		set_bpp(24);
		g_rdpScreen.blackPixel = 1;
		g_firstTime = 0;

		RRExtensionInit(); /* RANDR */
	}

	if (strcmp(argv[i], "-geometry") == 0)
	{
		if (i + 1 >= argc)
		{
			UseMsg();
		}

		if (sscanf(argv[i + 1], "%dx%d", &g_rdpScreen.width, &g_rdpScreen.height) != 2)
		{
			DEBUG_OUT("Invalid geometry %s\n", argv[i + 1]);
			UseMsg();
		}

		rdpWriteMonitorConfig(g_pScreen, g_rdpScreen.width, g_rdpScreen.height);

		return 2;
	}

	if (strcmp(argv[i], "-depth") == 0)
	{
		if (i + 1 >= argc)
		{
			UseMsg();
		}

		g_rdpScreen.depth = atoi(argv[i + 1]);

		if (set_bpp(g_rdpScreen.depth) != 0)
		{
			UseMsg();
		}

		return 2;
	}

	if (strcmp(argv[i], "-uds") == 0)
	{
		return 2;
	}

	return 0;
}

void OsVendorInit(void)
{

}

/* ddxInitGlobals - called by |InitGlobals| from os/util.c */
void ddxInitGlobals(void)
{

}

int XkbDDXSwitchScreen(DeviceIntPtr dev, KeyCode key, XkbAction *act)
{
	DEBUG_OUT("XkbDDXSwitchScreen:\n");
	return 1;
}

int XkbDDXPrivate(DeviceIntPtr dev, KeyCode key, XkbAction *act)
{
	DEBUG_OUT("XkbDDXPrivate:\n");
	return 0;
}

int XkbDDXTerminateServer(DeviceIntPtr dev, KeyCode key, XkbAction *act)
{
	DEBUG_OUT("XkbDDXTerminateServer:\n");
	GiveUp(1);
	return 0;
}

static ExtensionModule rdpExtensions[] =
{
#ifdef GLXEXT
	{ GlxExtensionInit, "GLX", &noGlxExtension },
#endif
};

static void rdpExtensionInit(void)
{
#if (XORG_VERSION_CURRENT >= XORG_VERSION(1,16,0))
	LoadExtensionList(rdpExtensions, ARRAYSIZE(rdpExtensions), TRUE);
#else
	int i;

	for (i = 0; i < ARRAYSIZE(rdpExtensions); i++)
	{
		LoadExtension(&rdpExtensions[i], TRUE);
	}
#endif
}

/* InitOutput is called every time the server resets.  It should call
   AddScreen for each screen (but we only ever have one), and in turn this
   will call rdpScreenInit. */
void InitOutput(ScreenInfo* pScreenInfo, int argc, char** argv)
{
	int i;
	int status;

	DEBUG_OUT("InitOutput:\n");
	g_initOutputCalled = 1;

	rdpExtensionInit();

	/* initialize pixmap formats */
	pScreenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
	pScreenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
	pScreenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
	pScreenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;
	pScreenInfo->numPixmapFormats = g_numFormats;

	for (i = 0; i < g_numFormats; i++)
	{
		pScreenInfo->formats[i] = g_formats[i];
	}

	if (!AddCallback(&ClientStateCallback, rdpClientStateChange, NULL))
	{
		ErrorF("InitOutput: AddCallback failed\n");
		return;
	}

	status = AddScreen(rdpScreenInit, argc, argv);

	if (status == -1)
	{
		FatalError("AddScreen failure\n");
	}

	DEBUG_OUT("InitOutput: out\n");
}

void InitInput(int argc, char** argv)
{
	int status;

	DEBUG_OUT("InitInput:\n");

	status = AllocDevicePair(serverClient, "X11rdp", &g_pointer, &g_keyboard,
			rdpMouseProc, rdpKeybdProc, 0);

	if (status != Success)
	{
		FatalError("Failed to init X11rdp default devices.\n");
	}

	mieqInit();
}

#if (XORG_VERSION_CURRENT >= XORG_VERSION(1,11,0))
void ddxGiveUp(enum ExitCode error)
#else
void ddxGiveUp()
#endif
{
	char unixSocketName[128];

	DEBUG_OUT("ddxGiveUp:\n");

	if (g_rdpScreen.pfbMemory)
		rdpScreenFrameBufferFree();

	if (g_initOutputCalled)
	{
		sprintf(unixSocketName, "/tmp/.X11-unix/X%s", display);
		unlink(unixSocketName);
	}
}

Bool LegalModifier(unsigned int key, DeviceIntPtr pDev)
{
	return TRUE;
}

void ProcessInputEvents(void)
{
	mieqProcessInputEvents();
}

void rfbRootPropertyChange(PropertyPtr pProp)
{

}

#if (XORG_VERSION_CURRENT >= XORG_VERSION(1,11,0))
void AbortDDX(enum ExitCode error)
{
	ddxGiveUp(error);
}
#else
void AbortDDX()
{
	ddxGiveUp();
}
#endif

#if (XORG_VERSION_CURRENT >= XORG_VERSION(1,13,0))
void OsVendorFatalError(const char *f, va_list args)
#else
void OsVendorFatalError(void)
#endif
{

}

/* print the command list parameters and exit the program */
void ddxUseMsg(void)
{
	ErrorF("\n");
	ErrorF("X11rdp specific options\n");
	ErrorF("-geometry WxH          set framebuffer width & height\n");
	ErrorF("-depth D               set framebuffer depth\n");
	ErrorF("\n");
	exit(1);
}

void OsVendorPreInit(void)
{

}

void CloseInput(void)
{
	DEBUG_OUT("CloseInput\n");
}

void DDXRingBell(int volume, int pitch, int duration)
{
	DEBUG_OUT("DDXRingBell\n");
}

void DeleteInputDeviceRequest(DeviceIntPtr dev)
{
	DEBUG_OUT("DeleteInputDeviceRequest\n");
}
