
cmake_minimum_required(VERSION 2.8)

set(MODULE_NAME "X11rdp")
set(MODULE_PREFIX "X11RDP")

set(X11RDP_PREFIX "/opt/X11rdp")
set(INCBASE "${X11RDP_PREFIX}/include")
set(LIBBASE "${X11RDP_PREFIX}/lib")
set(XSRCBASE "../X11rdp/Source/xorg-server")

set(${MODULE_PREFIX}_SRCS
	gcops.h
	rdpCopyArea.c
	rdpCopyArea.h
	rdpCopyPlane.c
	rdpCopyPlane.h
	rdpdraw.c
	rdpdraw.h
	rdpFillPolygon.c
	rdpFillPolygon.h
	rdpFillSpans.c
	rdpFillSpans.h
	rdp.h
	rdpImageGlyphBlt.c
	rdpImageGlyphBlt.h
	rdpImageText16.c
	rdpImageText16.h
	rdpImageText8.c
	rdpImageText8.h
	rdpinput.c
	rdpmain.c
	rdpmisc.c
	rdpPolyArc.c
	rdpPolyArc.h
	rdpPolyFillArc.c
	rdpPolyFillArc.h
	rdpPolyFillRect.c
	rdpPolyFillRect.h
	rdpPolyGlyphBlt.c
	rdpPolyGlyphBlt.h
	rdpPolylines.c
	rdpPolylines.h
	rdpPolyPoint.c
	rdpPolyPoint.h
	rdpPolyRectangle.c
	rdpPolyRectangle.h
	rdpPolySegment.c
	rdpPolySegment.h
	rdpPolyText16.c
	rdpPolyText16.h
	rdpPolyText8.c
	rdpPolyText8.h
	rdpPushPixels.c
	rdpPushPixels.h
	rdpPutImage.c
	rdpPutImage.h
	rdprandr.c
	rdprandr.h
	rdpSetSpans.c
	rdpSetSpans.h
	rdpup.c)

set(${MODULE_PREFIX}_SRCS ${${MODULE_PREFIX}_SRCS}
	../X11rdp/Source/xorg-server/mi/miinitext.c
	../X11rdp/Source/xorg-server/fb/fbcmap_mi.c)

add_library("dbe-static" STATIC IMPORTED)
set_property(TARGET "dbe-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/dbe/.libs/libdbe.a")

add_library("dix-static" STATIC IMPORTED)
set_property(TARGET "dix-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/dix/.libs/libdix.a")

add_library("main-static" STATIC IMPORTED)
set_property(TARGET "main-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/dix/.libs/libmain.a")

add_library("fb-static" STATIC IMPORTED)
set_property(TARGET "fb-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/fb/.libs/libfb.a")

add_library("mi-static" STATIC IMPORTED)
set_property(TARGET "mi-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/mi/.libs/libmi.a")

add_library("os-static" STATIC IMPORTED)
set_property(TARGET "os-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/os/.libs/libos.a")

add_library("randr-static" STATIC IMPORTED)
set_property(TARGET "randr-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/randr/.libs/librandr.a")

add_library("record-static" STATIC IMPORTED)
set_property(TARGET "record-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/record/.libs/librecord.a")

add_library("render-static" STATIC IMPORTED)
set_property(TARGET "render-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/render/.libs/librender.a")

add_library("xkb-static" STATIC IMPORTED)
set_property(TARGET "xkb-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/xkb/.libs/libxkb.a")

add_library("Xext-static" STATIC IMPORTED)
set_property(TARGET "Xext-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/Xext/.libs/libXext.a")

add_library("Xi-static" STATIC IMPORTED)
set_property(TARGET "Xi-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/Xi/.libs/libXi.a")

add_library("glx-static" STATIC IMPORTED)
set_property(TARGET "glx-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/glx/.libs/libglx.a")

add_library("xfixes-static" STATIC IMPORTED)
set_property(TARGET "xfixes-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/xfixes/.libs/libxfixes.a")

add_library("damageext-static" STATIC IMPORTED)
set_property(TARGET "damageext-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/damageext/.libs/libdamageext.a")

add_library("damage-static" STATIC IMPORTED)
set_property(TARGET "damage-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/miext/damage/.libs/libdamage.a")

set(${MODULE_PREFIX}_STATIC_LIBS
	dbe-static dix-static main-static fb-static mi-static os-static randr-static record-static
	render-static xkb-static Xext-static Xi-static glx-static xfixes-static damageext-static damage-static)

link_directories(${LIBBASE})

set(${MODULE_PREFIX}_INCLUDES
	"../../include"
	"../../cfb"
	"../../mfb"
	"../../mi"
	"${INCBASE}"
	"${INCBASE}/X11"
	"${INCBASE}/X11/fonts"
	"${INCBASE}/X11/extensions"
	"${INCBASE}/xorg"
	"${INCBASE}/pixman-1"
	"../../os"
	"../../render"
	"../xfree86/common"
	"../xfree86/os-support"
	"../../../common"
	"${XSRCBASE}/Xext"
	"${XSRCBASE}/composite"
	"${XSRCBASE}/damageext"
	"${XSRCBASE}/xfixes"
	"${XSRCBASE}/Xi"
	"${XSRCBASE}/mi"
	"${XSRCBASE}/miext/damage"
	"${XSRCBASE}/render"
	"${XSRCBASE}/randr"
	"${XSRCBASE}/fb")

include_directories(${${MODULE_PREFIX}_INCLUDES})

set(${MODULE_PREFIX}_DEFINES
	"-D_SVID_SOURCE -D_REENTRANT 
	-DGLX_USE_MESA -DXRECORD -D_GNU_SOURCE -DXAPPGROUP 
	-DTOGCUP -DSINGLEDEPTH -DXFree86Server 
	-DDBE -DEVI -DXVMC -DFONTCACHE -DGCCUSESGAS -DSTATIC_COLOR 
	-DAVOID_GLYPHBLT -DFUNCPROTO=15 -DNARROWPROTO -DDDXOSFATALERROR 
	-DPART_NET -DDDXTIME -D_HAVE_XALLOC_DECLS -DDAMAGE -DXFIXES 
	-DSMART_SCHEDULE -DSERVER_LOCK -DGLXEXT -DSHAPE -DRENDER -DRANDR 
	-DBIGREQS -D_POSIX_SOURCE -D_BSD_SOURCE -DSCREENSAVER 
	-DXCMISC -DXTEST -DXTRAP -DXV -DXSYNC -DMITMISC -DPANORAMIX 
	-DDPMSExtension -DXvExtension -DXvMCExtension -DXResExtension 
	-DMITSHM -DPIXPRIV -DNDEBUG -DDDXOSINIT -DXKB -DXINPUT")

add_definitions(-D_POSIX_C_SOURCE=199309L)
add_definitions(${${MODULE_PREFIX}_DEFINES})

add_executable(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set(${MODULE_PREFIX}_LIBS
	freetype z m Xfont Xau Xdmcp pixman-1 rt dl crypto GL Xdamage)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS} ${${MODULE_PREFIX}_STATIC_LIBS})

