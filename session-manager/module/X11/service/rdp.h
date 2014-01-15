/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2005-2013 Jay Sorg
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

#ifndef FREERDS_X11RDP_MAIN_H
#define FREERDS_X11RDP_MAIN_H

#include "xorg-config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>

#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/fonts/fontstruct.h>
#include <X11/extensions/XKBstr.h>

#include "scrnintstr.h"
#include "servermd.h"
#include "colormapst.h"
#include "gcstruct.h"
#include "input.h"
#include "mipointer.h"
#include "dixstruct.h"
#include "propertyst.h"
#include "dix.h"
#include "dixfontstr.h"
#include "cursorstr.h"
#include "picturestr.h"
#include "inputstr.h"
#include "randrstr.h"
#include "mi.h"
#include "fb.h"
#include "micmap.h"
#include "events.h"
#include "exevents.h"
#include "xserver-properties.h"
#include "xkbsrv.h"

#include <winpr/crt.h>
#include <winpr/stream.h>

#include <freerdp/freerdp.h>
#include <freerds/backend.h>

#define XORG_VERSION(_major, _minor, _patch) (((_major) * 10000000) + ((_minor) * 100000) + ((_patch) * 1000) + 0)

#define X11RDPVER "0.7.7"

#define PixelDPI 96
#define PixelToMM(_size) ((int)(((((double)(_size)) / PixelDPI) * 25.4 + 0.5)))

struct _rdpScreenInfoRec
{
	int width;
	int height;
	int depth;
	int scanline;
	int bitsPerPixel;
	int bytesPerPixel;
	int sizeInBytes;
	char* pfbMemory;

	Pixel blackPixel;
	Pixel whitePixel;

	/* wrapped screen functions */

	/* Random screen procedures */
	CloseScreenProcPtr CloseScreen;

	/* GC procedures */
	CreateGCProcPtr CreateGC;

	/* Pixmap procedures */
	CreatePixmapProcPtr CreatePixmap;
	DestroyPixmapProcPtr DestroyPixmap;

	/* Window Procedures */
	CreateWindowProcPtr CreateWindow;
	DestroyWindowProcPtr DestroyWindow;
	PositionWindowProcPtr PositionWindow;
	RealizeWindowProcPtr RealizeWindow;
	UnrealizeWindowProcPtr UnrealizeWindow;
	ChangeWindowAttributesProcPtr ChangeWindowAttributes;
	WindowExposuresProcPtr WindowExposures;

	/* Colormap procedures */
	CreateColormapProcPtr CreateColormap;
	DestroyColormapProcPtr DestroyColormap;
	InstallColormapProcPtr InstallColormap;
	UninstallColormapProcPtr UninstallColormap;
	ListInstalledColormapsProcPtr ListInstalledColormaps;
	StoreColorsProcPtr StoreColors;
	ResolveColorProcPtr ResolveColor;

	CopyWindowProcPtr CopyWindow;
	ClearToBackgroundProcPtr ClearToBackground;
	ScreenWakeupHandlerProcPtr WakeupHandler;
	CompositeProcPtr Composite;
	GlyphsProcPtr Glyphs;

	int segmentId;
	int fbAttached;

	int rdp_bpp;
	int rdp_Bpp;
	int rdp_Bpp_mask;
};
typedef struct _rdpScreenInfoRec rdpScreenInfoRec;
typedef rdpScreenInfoRec* rdpScreenInfoPtr;

struct _rdpGCRec
{
	GCFuncs* funcs;
	GCOps* ops;
};
typedef struct _rdpGCRec rdpGCRec;
typedef rdpGCRec* rdpGCPtr;

#define GETGCPRIV(_pGC) \
		(rdpGCPtr)dixGetPrivateAddr(&(_pGC->devPrivates), &g_rdpGCIndex)

struct _rdpWindowRec
{
	int status;
};
typedef struct _rdpWindowRec rdpWindowRec;
typedef rdpWindowRec* rdpWindowPtr;

#define GETWINPRIV(_pWindow) \
		(rdpWindowPtr)dixGetPrivateAddr(&(_pWindow->devPrivates), &g_rdpWindowIndex)

struct _rdpPixmapRec
{
	int status;
	int pad0;
	int kind_width;
};
typedef struct _rdpPixmapRec rdpPixmapRec;
typedef rdpPixmapRec* rdpPixmapPtr;

#define GETPIXPRIV(_pPixmap) \
		(rdpPixmapPtr)dixGetPrivateAddr(&(_pPixmap->devPrivates), &g_rdpPixmapIndex)

#include "rdpDraw.h"
#include "rdpInput.h"
#include "rdpUpdate.h"

#endif /* FREERDS_X11RDP_MAIN_H */
