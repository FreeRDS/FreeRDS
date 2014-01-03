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

This is the main file called from main.c
Sets up the  functions

 */

#include "rdp.h"
#include "rdprandr.h"

#include "glx_extinit.h"

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

rdpScreenInfoRec g_rdpScreen; /* the one screen */
ScreenPtr g_pScreen = 0;

DevPrivateKeyRec g_rdpGCIndex;
DevPrivateKeyRec g_rdpWindowIndex;
DevPrivateKeyRec g_rdpPixmapIndex;

/* main mouse and keyboard devices */
DeviceIntPtr g_pointer = 0;
DeviceIntPtr g_keyboard = 0;

Bool g_wrapWindow = 1;
Bool g_wrapPixmap = 1;

rdpPixmapRec g_screenPriv;

/* if true, running in RemoteApp / RAIL mode */
int g_use_rail = 0;

int g_con_number = 0; /* increments for each connection */

WindowPtr g_invalidate_window = 0;

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
	rdpPointerWarpCursor,
	rdpPointerEnqueueEvent,
	rdpPointerNewEventScreen
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

static void rdpWakeupHandler(ScreenPtr pScreen, unsigned long result, pointer pReadmask)
{
	pScreen->WakeupHandler = g_rdpScreen.WakeupHandler;
	pScreen->WakeupHandler(pScreen, result, pReadmask);
	pScreen->WakeupHandler = rdpWakeupHandler;
}

static void rdpBlockHandler1(pointer blockData, OSTimePtr pTimeout, pointer pReadmask)
{
}

static void rdpWakeupHandler1(pointer blockData, int result, pointer pReadmask)
{
	rdpup_check();
}

#if 0
static Bool
rdpDeviceCursorInitialize(DeviceIntPtr pDev, ScreenPtr pScreen)
{
	DEBUG_OUT("rdpDeviceCursorInitializeProcPtr:\n");
	return 1;
}

static void
rdpDeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScreen)
{
	DEBUG_OUT("rdpDeviceCursorCleanupProcPtr:\n");
}
#endif

#if 0
Bool
rdpCreateColormap(ColormapPtr pCmap)
{
	DEBUG_OUT("rdpCreateColormap:\n");
	return 1;
}

static void
rdpDestroyColormap(ColormapPtr pColormap)
{
	DEBUG_OUT("rdpDestroyColormap:\n");
}
#endif

/* returns boolean, true if everything is ok */
static Bool rdpScreenInit(ScreenPtr pScreen, int argc, char** argv)
{
	int dpix;
	int dpiy;
	int ret;
	Bool vis_found;
	VisualPtr vis;
	PictureScreenPtr ps;

	g_pScreen = pScreen;
	ZeroMemory(&g_screenPriv, sizeof(g_screenPriv));

	dpix = PixelDPI;
	dpiy = PixelDPI;
	monitorResolution = PixelDPI;

	g_rdpScreen.paddedWidthInBytes = PixmapBytePad(g_rdpScreen.width, g_rdpScreen.depth);
	g_rdpScreen.bitsPerPixel = rdpBitsPerPixel(g_rdpScreen.depth);

	DEBUG_OUT("\n");
	ErrorF("X11rdp, an X server for xrdp\n");
	ErrorF("Version %s\n", X11RDPVER);
	ErrorF("Copyright (C) 2005-2012 Jay Sorg\n");
	ErrorF("Copyright (C) 2013 Thincast Technologies GmbH\n");
	ErrorF("See http://xrdp.sf.net for information on xrdp.\n");
#if defined(XORG_VERSION_CURRENT) && defined (XVENDORNAME)
	ErrorF("Underlying X server release %d, %s\n",
			XORG_VERSION_CURRENT, XVENDORNAME);
#endif
#if defined(XORG_RELEASE)
	ErrorF("Xorg %s\n", XORG_RELEASE);
#endif
	ErrorF("Screen width %d height %d depth %d bpp %d\n", g_rdpScreen.width,
			g_rdpScreen.height, g_rdpScreen.depth, g_rdpScreen.bitsPerPixel);
	ErrorF("dpix %d dpiy %d\n", dpix, dpiy);

	g_rdpScreen.sharedMemory = 1;

	if (!g_rdpScreen.pfbMemory)
	{
		g_rdpScreen.sizeInBytes = (g_rdpScreen.paddedWidthInBytes * g_rdpScreen.height);

		if (g_rdpScreen.sharedMemory)
		{
			/* allocate shared memory segment */
			g_rdpScreen.segmentId = shmget(IPC_PRIVATE, g_rdpScreen.sizeInBytes,
					IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

			/* attach the shared memory segment */
			g_rdpScreen.pfbMemory = (char*) shmat(g_rdpScreen.segmentId, 0, 0);

			DEBUG_OUT("sizeInBytes %d segmentId: %d pfbMemory: %p\n",
					g_rdpScreen.sizeInBytes, g_rdpScreen.segmentId, g_rdpScreen.pfbMemory);
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

	miClearVisualTypes();

	if (defaultColorVisualClass == -1)
	{
		defaultColorVisualClass = TrueColor;
	}

	if (!miSetVisualTypes(g_rdpScreen.depth,
			miGetDefaultVisualMask(g_rdpScreen.depth),
			8, defaultColorVisualClass))
	{
		rdpLog("rdpScreenInit miSetVisualTypes failed\n");
		return 0;
	}

	miSetPixmapDepths();

	switch (g_rdpScreen.bitsPerPixel)
	{
		case 8:
			ret = fbScreenInit(pScreen, g_rdpScreen.pfbMemory,
					g_rdpScreen.width, g_rdpScreen.height,
					dpix, dpiy, g_rdpScreen.paddedWidthInBytes, 8);
			break;

		case 16:
			ret = fbScreenInit(pScreen, g_rdpScreen.pfbMemory,
					g_rdpScreen.width, g_rdpScreen.height,
					dpix, dpiy, g_rdpScreen.paddedWidthInBytes / 2, 16);
			break;

		case 32:
			ret = fbScreenInit(pScreen, g_rdpScreen.pfbMemory,
					g_rdpScreen.width, g_rdpScreen.height,
					dpix, dpiy, g_rdpScreen.paddedWidthInBytes / 4, 32);
			break;

		default:
			DEBUG_OUT("rdpScreenInit: error\n");
			return 0;
	}

	if (!ret)
	{
		DEBUG_OUT("rdpScreenInit: error\n");
		return 0;
	}

	/* TODO: port miInitializeBackingStore */
	//miInitializeBackingStore(pScreen);

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

	/* GC procedures */
	g_rdpScreen.CreateGC = pScreen->CreateGC;

	/* Pixmap procedures */
	g_rdpScreen.CreatePixmap = pScreen->CreatePixmap;
	g_rdpScreen.DestroyPixmap = pScreen->DestroyPixmap;

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

	/* Backing store procedures */
	//g_rdpScreen.RestoreAreas = pScreen->RestoreAreas;

	/* os layer procedures */
	g_rdpScreen.WakeupHandler = pScreen->WakeupHandler;

	g_rdpScreen.CreateColormap = pScreen->CreateColormap;
	g_rdpScreen.DestroyColormap = pScreen->DestroyColormap;

	ps = GetPictureScreenIfSet(pScreen);

	if (ps)
	{
		g_rdpScreen.Composite = ps->Composite;
		g_rdpScreen.Glyphs = ps->Glyphs;
	}

	pScreen->blackPixel = g_rdpScreen.blackPixel;
	pScreen->whitePixel = g_rdpScreen.whitePixel;

	/* Random screen procedures */
	pScreen->CloseScreen = rdpCloseScreen;
	pScreen->QueryBestSize = rdpQueryBestSize;
	pScreen->WakeupHandler = rdpWakeupHandler;

	if (ps)
	{
		ps->Composite = rdpComposite;
		ps->Glyphs = rdpGlyphs;
	}

	pScreen->SaveScreen = rdpSaveScreen;
	/* GC procedures */
	pScreen->CreateGC = rdpCreateGC;

	if (g_wrapPixmap)
	{
		/* Pixmap procedures */
		pScreen->CreatePixmap = rdpCreatePixmap;
		pScreen->DestroyPixmap = rdpDestroyPixmap;
	}

	if (g_wrapWindow)
	{
		/* Window Procedures */
		pScreen->CreateWindow = rdpCreateWindow;
		pScreen->DestroyWindow = rdpDestroyWindow;
		pScreen->ChangeWindowAttributes = rdpChangeWindowAttributes;
		pScreen->RealizeWindow = rdpRealizeWindow;
		pScreen->UnrealizeWindow = rdpUnrealizeWindow;
		pScreen->PositionWindow = rdpPositionWindow;
		pScreen->WindowExposures = rdpWindowExposures;
	}

	pScreen->CopyWindow = rdpCopyWindow;
	pScreen->ClearToBackground = rdpClearToBackground;

	/* Backing store procedures */
	//pScreen->RestoreAreas = rdpRestoreAreas;

#if 0
	pScreen->CreateColormap = rdpCreateColormap;
	pScreen->DestroyColormap = rdpDestroyColormap;
#endif

	miPointerInitialize(pScreen, &g_rdpSpritePointerFuncs, &g_rdpPointerCursorFuncs, 1);

#if 0
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
		rdpLog("rdpScreenInit: couldn't find root visual\n");
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
		ret = rdpup_init();

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

		if (sscanf(argv[i + 1], "%dx%d", &g_rdpScreen.width,
				&g_rdpScreen.height) != 2)
		{
			DEBUG_OUT("Invalid geometry %s\n", argv[i + 1]);
			UseMsg();
		}

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
	int i;

	for (i = 0; i < ARRAY_SIZE(rdpExtensions); i++)
	{
		LoadExtension(&rdpExtensions[i], TRUE);
	}
}

/* InitOutput is called every time the server resets.  It should call
   AddScreen for each screen (but we only ever have one), and in turn this
   will call rdpScreenInit. */
void InitOutput(ScreenInfo* pScreenInfo, int argc, char** argv)
{
	int i;

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
		rdpLog("InitOutput: AddCallback failed\n");
		return;
	}

	/* initialize screen */
	if (AddScreen(rdpScreenInit, argc, argv) == -1)
	{
		FatalError("Couldn't add screen\n");
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

void ddxGiveUp(enum ExitCode error)
{
	char unixSocketName[128];

	DEBUG_OUT("ddxGiveUp:\n");

	if (g_rdpScreen.sharedMemory)
	{
		if (g_rdpScreen.pfbMemory)
		{
			/* detach shared memory segment */
			shmdt(g_rdpScreen.pfbMemory);
			g_rdpScreen.pfbMemory = NULL;

			/* deallocate shared memory segment */
			shmctl(g_rdpScreen.segmentId, IPC_RMID, 0);
		}
	}
	else
	{
		free(g_rdpScreen.pfbMemory);
		g_rdpScreen.pfbMemory = NULL;
	}

	if (g_initOutputCalled)
	{
		sprintf(unixSocketName, "/tmp/.X11-unix/X%s", display);
		unlink(unixSocketName);
	}
}

Bool LegalModifier(unsigned int key, DeviceIntPtr pDev)
{
	return 1; /* true */
}

void ProcessInputEvents(void)
{
	mieqProcessInputEvents();
}

/* needed for some reason? todo
   needs to be rfb */
void rfbRootPropertyChange(PropertyPtr pProp)
{

}

void AbortDDX(enum ExitCode error)
{
	ddxGiveUp(error);
}

void OsVendorFatalError(const char *f, va_list args)
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
