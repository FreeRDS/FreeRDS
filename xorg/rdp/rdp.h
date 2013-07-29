/*
Copyright 2005-2013 Jay Sorg

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

#include <xrdp-ng/xrdp.h>

#define XORG_VERSION(_major, _minor, _patch) (((_major) * 10000000) + ((_minor) * 100000) + ((_patch) * 1000) + 0)

/* test to see if this is xorg source or xfree86 */
#ifdef XORGSERVER
#  define RDP_IS_XORG
#else
#  include <xf86Version.h>
#  if (XF86_VERSION_MAJOR == 4 && XF86_VERSION_MINOR > 3)
#    define RDP_IS_XFREE86
#  elif (XF86_VERSION_MAJOR > 4)
#    define RDP_IS_XFREE86
#  else
#    define RDP_IS_XORG
#  endif
#endif

#define X11RDPVER "0.7.7"

#define PixelDPI 100
#define PixelToMM(_size) (((_size) * 254 + (PixelDPI) * 5) / ((PixelDPI) * 10))

struct image_data
{
	int width;
	int height;
	int bpp;
	int Bpp;
	int lineBytes;
	char* pixels;
};

/* Per-screen (framebuffer) structure.  There is only one of these, since we
   don't allow the X server to have multiple screens. */
struct _rdpScreenInfoRec
{
	int width;
	int paddedWidthInBytes;
	int height;
	int depth;
	int bitsPerPixel;
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

	CreateColormapProcPtr CreateColormap;
	DestroyColormapProcPtr DestroyColormap;

	CopyWindowProcPtr CopyWindow;
	ClearToBackgroundProcPtr ClearToBackground;
	ScreenWakeupHandlerProcPtr WakeupHandler;
	CompositeProcPtr Composite;
	GlyphsProcPtr Glyphs;

	/* TODO: port old code using backing store procedures */

	/* Backing store procedures */
	//RestoreAreasProcPtr RestoreAreas;

	int segmentId;
	int sharedMemory;
	int fbAttached;

	int rdp_width;
	int rdp_height;
	int rdp_bpp;
	int rdp_Bpp;
	int rdp_Bpp_mask;

	int Jpeg;
	int NSCodec;
	int RemoteFX;
	int CodecMode;
	int OffscreenSupportLevel;
	int OffscreenCacheSize;
	int OffscreenCacheEntries;
	int RailSupportLevel;
	int PointerFlags;
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

#define XR_IS_ROOT(_pWindow) ((_pWindow)->drawable.pScreen->root == (_pWindow))

/* for tooltips */
#define XR_STYLE_TOOLTIP (0x80000000)
#define XR_EXT_STYLE_TOOLTIP (0x00000080 | 0x00000008)

/* for normal desktop windows */
/* WS_TILEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
                   WS_MINIMIZEBOX | WS_MAXIMIZEBOX) */
#define XR_STYLE_NORMAL (0x00C00000 | 0x00080000 | 0x00040000 | 0x00010000 | 0x00020000)
#define XR_EXT_STYLE_NORMAL (0x00040000)

/* for dialogs */
#define XR_STYLE_DIALOG (0x80000000)
#define XR_EXT_STYLE_DIALOG (0x00040000)

#define RDI_FILL 1
#define RDI_IMGLL 2 /* lossless */
#define RDI_IMGLY 3 /* lossy */
#define RDI_LINE 4
#define RDI_SCRBLT 5

struct urdp_draw_item_fill
{
	int opcode;
	int fg_color;
	int bg_color;
	int pad0;
};

struct urdp_draw_item_img
{
	int opcode;
	int pad0;
};

struct urdp_draw_item_line
{
	int opcode;
	int fg_color;
	int bg_color;
	int width;
	xSegment* segs;
	int nseg;
	int flags;
};

struct urdp_draw_item_scrblt
{
	int srcx;
	int srcy;
	int dstx;
	int dsty;
	int cx;
	int cy;
};

union urdp_draw_item
{
	struct urdp_draw_item_fill fill;
	struct urdp_draw_item_img img;
	struct urdp_draw_item_line line;
	struct urdp_draw_item_scrblt scrblt;
};

struct rdp_draw_item
{
	int type;
	int flags;
	struct rdp_draw_item* prev;
	struct rdp_draw_item* next;
	RegionPtr reg;
	union urdp_draw_item u;
};

struct _rdpPixmapRec
{
	int status;
	int rdpindex;
	int con_number;
	int is_dirty;
	int pad0;
	int kind_width;
	struct rdp_draw_item* draw_item_head;
	struct rdp_draw_item* draw_item_tail;
};
typedef struct _rdpPixmapRec rdpPixmapRec;
typedef rdpPixmapRec* rdpPixmapPtr;
#define GETPIXPRIV(_pPixmap) \
		(rdpPixmapPtr)dixGetPrivateAddr(&(_pPixmap->devPrivates), &g_rdpPixmapIndex)

#define XRDP_IS_OS(_priv) (_priv->status != 0)

/* rdpmisc.c */

#include "os_calls.h"

void rdpLog(char *format, ...);
int rdpBitsPerPixel(int depth);
void rdpClientStateChange(CallbackListPtr* cbl, pointer myData, pointer clt);
void RegionAroundSegs(RegionPtr reg, xSegment* segs, int nseg);

/* rdpdraw.c */
Bool rdpCloseScreen(int i, ScreenPtr pScreen);

int draw_item_add(rdpPixmapRec* priv, struct rdp_draw_item* di);
int draw_item_remove(rdpPixmapRec* priv, struct rdp_draw_item* di);
int draw_item_remove_all(rdpPixmapRec* priv);
int draw_item_pack(PixmapPtr pix, rdpPixmapRec* priv);
int draw_item_add_img_region(rdpPixmapRec* priv, RegionPtr reg, int opcode, int type);
int draw_item_add_fill_region(rdpPixmapRec* priv, RegionPtr reg, int color, int opcode);
int draw_item_add_line_region(rdpPixmapRec* priv, RegionPtr reg, int color,
		int opcode, int width, xSegment* segs, int nsegs,
		int is_segment);
int draw_item_add_srcblt_region(rdpPixmapRec* priv, RegionPtr reg,
		int srcx, int srcy, int dstx, int dsty,
		int cx, int cy);

PixmapPtr rdpCreatePixmap(ScreenPtr pScreen, int width, int height, int depth, unsigned usage_hint);
Bool rdpDestroyPixmap(PixmapPtr pPixmap);
int xrdp_is_os(PixmapPtr pix, rdpPixmapPtr priv);
Bool rdpCreateWindow(WindowPtr pWindow);
Bool rdpDestroyWindow(WindowPtr pWindow);
Bool rdpPositionWindow(WindowPtr pWindow, int x, int y);
Bool rdpRealizeWindow(WindowPtr pWindow);
Bool rdpUnrealizeWindow(WindowPtr pWindow);
Bool rdpChangeWindowAttributes(WindowPtr pWindow, unsigned long mask);
void rdpWindowExposures(WindowPtr pWindow, RegionPtr pRegion, RegionPtr pBSRegion);

Bool rdpCreateGC(GCPtr pGC);
void rdpCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr pOldRegion);
void rdpClearToBackground(WindowPtr pWin, int x, int y, int w, int h, Bool generateExposures);
RegionPtr rdpRestoreAreas(WindowPtr pWin, RegionPtr prgnExposed);
void rdpInstallColormap(ColormapPtr pmap);
void rdpUninstallColormap(ColormapPtr pmap);
int rdpListInstalledColormaps(ScreenPtr pScreen, Colormap* pmaps);
void rdpStoreColors(ColormapPtr pmap, int ndef, xColorItem* pdefs);
Bool rdpSaveScreen(ScreenPtr pScreen, int on);
Bool rdpRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
Bool rdpUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
void rdpCursorLimits(ScreenPtr pScreen, CursorPtr pCursor, BoxPtr pHotBox, BoxPtr pTopLeftBox);
void rdpConstrainCursor(ScreenPtr pScreen, BoxPtr pBox);
Bool rdpSetCursorPosition(ScreenPtr pScreen, int x, int y, Bool generateEvent);
Bool rdpDisplayCursor(ScreenPtr pScreen, CursorPtr pCursor);
void rdpRecolorCursor(ScreenPtr pScreen, CursorPtr pCursor, Bool displayed);
void rdpComposite(CARD8 op, PicturePtr pSrc, PicturePtr pMask, PicturePtr pDst,
		INT16 xSrc, INT16 ySrc, INT16 xMask, INT16 yMask, INT16 xDst,
		INT16 yDst, CARD16 width, CARD16 height);
void rdpGlyphs(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
		PictFormatPtr maskFormat,
		INT16 xSrc, INT16 ySrc, int nlists, GlyphListPtr lists,
		GlyphPtr* glyphs);

/* rdpinput.c */
int rdpKeybdProc(DeviceIntPtr pDevice, int onoff);
int rdpMouseProc(DeviceIntPtr pDevice, int onoff);
Bool rdpCursorOffScreen(ScreenPtr* ppScreen, int* x, int* y);
void rdpCrossScreen(ScreenPtr pScreen, Bool entering);
void rdpPointerWarpCursor(DeviceIntPtr pDev, ScreenPtr pScr, int x, int y);
void rdpPointerEnqueueEvent(DeviceIntPtr pDev, InternalEvent* event);
void rdpPointerNewEventScreen(DeviceIntPtr pDev, ScreenPtr pScr, Bool fromDIX);
Bool rdpSpriteRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScr, CursorPtr pCurs);
Bool rdpSpriteUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScr, CursorPtr pCurs);
void rdpSpriteSetCursor(DeviceIntPtr pDev, ScreenPtr pScr, CursorPtr pCurs, int x, int y);
void rdpSpriteMoveCursor(DeviceIntPtr pDev, ScreenPtr pScr, int x, int y);
Bool rdpSpriteDeviceCursorInitialize(DeviceIntPtr pDev, ScreenPtr pScr);
void rdpSpriteDeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScr);
void PtrAddEvent(int buttonMask, int x, int y);
void KbdAddEvent(int down, int param1, int param2, int param3, int param4);
void KbdSync(int param1);

/* rdpup.c */
int rdpup_add_os_bitmap(PixmapPtr pixmap, rdpPixmapPtr priv);
int rdpup_remove_os_bitmap(int rdpindex);
void rdpup_get_screen_image_rect(struct image_data* id);
void rdpup_get_pixmap_image_rect(PixmapPtr pPixmap, struct image_data* id);
UINT32 rdpup_convert_color(UINT32 color);
UINT32 rdpup_convert_opcode(int opcode);
UINT32 rdp_dstblt_rop(int opcode);
int rdpup_init(void);
int rdpup_check(void);
int rdpup_begin_update(void);
int rdpup_end_update(void);
int rdpup_opaque_rect(XRDP_MSG_OPAQUE_RECT* msg);
int rdpup_screen_blt(short x, short y, int cx, int cy, short srcx, short srcy);
int rdpup_patblt(XRDP_MSG_PATBLT* msg);
int rdpup_dstblt(XRDP_MSG_DSTBLT* msg);
int rdpup_set_clipping_region(XRDP_MSG_SET_CLIPPING_REGION* msg);
int rdpup_set_clip(short x, short y, int cx, int cy);
int rdpup_reset_clip(void);
int rdpup_set_opcode(int opcode);
int rdpup_draw_line(XRDP_MSG_LINE_TO* msg);
void rdpup_send_area(struct image_data* id, int x, int y, int w, int h);
void rdpup_send_area_codec(struct image_data* id, int x, int y, int w, int h);
int rdpup_set_pointer(XRDP_MSG_SET_POINTER* msg);
int rdpup_create_os_surface(int rdpindexd, int width, int height);
int rdpup_switch_os_surface(int rdpindex);
int rdpup_delete_os_surface(int rdpindex);
void rdpup_paint_rect_os(int x, int y, int cx, int cy, int rdpindex, int srcx, int srcy);
void rdpup_create_window(WindowPtr pWindow, rdpWindowRec* priv);
void rdpup_delete_window(WindowPtr pWindow, rdpWindowRec* priv);
int rdpup_check_dirty(PixmapPtr pDirtyPixmap, rdpPixmapRec* pDirtyPriv);
int rdpup_check_dirty_screen(rdpPixmapRec* pDirtyPriv);

void rdpup_shared_framebuffer(XRDP_MSG_SHARED_FRAMEBUFFER* msg);

void rdpScheduleDeferredUpdate(void);

