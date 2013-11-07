# FreeRDS: FreeRDP Remote Desktop Services
# FreeRDS cmake build script
#
# Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
# Copyright 2013 Thinstuff Technologies GmbH
# Copyright 2013 Bernhard Miklautz <bmiklautz@thinstuff.at>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
project(X11rdp)

include(GNUInstallDirs)
include(MergeStaticLibs)
include(FindPackageHandleStandardArgs)

# X11rdp

set(MODULE_NAME "X11rdp")
set(MODULE_PREFIX "X11RDP")

set(XSRCBASE_RELATIVE "xorg-server")
set(XOBJBASE_RELATIVE "xorg-server")

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${XSRCBASE_RELATIVE}")
	message(FATAL_ERROR "xorg-server not found - see xorg-build/README how to build/prepare")
endif()

message(STATUS "Using xorg-server sources in ${XSRCBASE_RELATIVE}")

# include local settings to overwrite/set some build options
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/LocalConfigSettings.cmake)
	message(STATUS "Using local settings")
	include(${CMAKE_CURRENT_SOURCE_DIR}/LocalConfigSettings.cmake)
else()
endif()

set(XSRCBASE "${CMAKE_CURRENT_SOURCE_DIR}/${XSRCBASE_RELATIVE}")
set(XOBJBASE "${CMAKE_CURRENT_SOURCE_DIR}/${XOBJBASE_RELATIVE}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -O2 -Wall -fno-strength-reduce")

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)

find_package(Threads REQUIRED)

# Xfont

find_path(XFONT_INCLUDE_DIR NAMES X11/fonts/fontconf.h)

find_library(XFONT_LIBRARY NAMES Xfont)

find_package_handle_standard_args(xfont DEFAULT_MSG XFONT_LIBRARY XFONT_INCLUDE_DIR)

if(XFONT_FOUND)
	set(XFONT_LIBRARIES ${XFONT_LIBRARY})
	set(XFONT_INCLUDE_DIRS ${XFONT_INCLUDE_DIR})
endif()

mark_as_advanced(XFONT_INCLUDE_DIR XFONT_LIBRARY)

# Xau

find_path(XAU_INCLUDE_DIR NAMES X11/Xauth.h)

find_library(XAU_LIBRARY NAMES Xau)

find_package_handle_standard_args(xau DEFAULT_MSG XAU_LIBRARY XAU_INCLUDE_DIR)

if(XAU_FOUND)
	set(XAU_LIBRARIES ${XAU_LIBRARY})
	set(XAU_INCLUDE_DIRS ${XAU_INCLUDE_DIR})
endif()

mark_as_advanced(XAU_INCLUDE_DIR XAU_LIBRARY)

# Xdmcp

find_path(XDMCP_INCLUDE_DIR NAMES X11/Xdmcp.h)

find_library(XDMCP_LIBRARY NAMES Xdmcp)

find_package_handle_standard_args(xdmcp DEFAULT_MSG XDMCP_LIBRARY XDMCP_INCLUDE_DIR)

if(XDMCP_FOUND)
	set(XDMCP_LIBRARIES ${XDMCP_LIBRARY})
	set(XDMCP_INCLUDE_DIRS ${XDMCP_INCLUDE_DIR})
endif()

mark_as_advanced(XDMCP_INCLUDE_DIR XDMCP_LIBRARY)

# GL

find_path(GL_INCLUDE_DIR NAMES GL/gl.h)

find_library(GL_LIBRARY NAMES GL)

find_package_handle_standard_args(GL DEFAULT_MSG GL_LIBRARY GL_INCLUDE_DIR)

if(GL_FOUND)
	set(GL_LIBRARIES ${GL_LIBRARY})
	set(GL_INCLUDE_DIRS ${GL_INCLUDE_DIR})
endif()

mark_as_advanced(GL_INCLUDE_DIR GL_LIBRARY)

include_directories(${XFONT_INCLUDE_DIRS} ${XAU_INCLUDE_DIRS} ${GL_INCLUDE_DIRS}
	${XDMCP_INCLUDE_DIRS} ${XDAMAGE_INCLUDE_DIRS} ${PIXMAN_INCLUDE_DIRS})

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

set(${MODULE_PREFIX}_XORG_XSERVER_SRCS
	${XSRCBASE}/mi/miinitext.c
	${XSRCBASE}/fb/fbcmap_mi.c)

set_source_files_properties(${${MODULE_PREFIX}_XORG_XSERVER_SRCS} GENERATED)

set(${MODULE_PREFIX}_SRCS ${${MODULE_PREFIX}_SRCS}
	${${MODULE_PREFIX}_XORG_XSERVER_SRCS})

add_library("composite-static" STATIC IMPORTED)
set_property(TARGET "composite-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/composite/.libs/libcomposite.a")

add_library("dbe-static" STATIC IMPORTED)
set_property(TARGET "dbe-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/dbe/.libs/libdbe.a")

add_library("dix-static" STATIC IMPORTED)
set_property(TARGET "dix-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/dix/.libs/libdix.a")

add_library("main-static" STATIC IMPORTED)
set_property(TARGET "main-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/dix/.libs/libmain.a")

add_library("fb-static" STATIC IMPORTED)
set_property(TARGET "fb-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/fb/.libs/libfb.a")

add_library("mi-static" STATIC IMPORTED)
set_property(TARGET "mi-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/mi/.libs/libmi.a")

add_library("os-static" STATIC IMPORTED)
set_property(TARGET "os-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/os/.libs/libos.a")

add_library("randr-static" STATIC IMPORTED)
set_property(TARGET "randr-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/randr/.libs/librandr.a")

add_library("record-static" STATIC IMPORTED)
set_property(TARGET "record-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/record/.libs/librecord.a")

add_library("render-static" STATIC IMPORTED)
set_property(TARGET "render-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/render/.libs/librender.a")

add_library("xkb-static" STATIC IMPORTED)
set_property(TARGET "xkb-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/xkb/.libs/libxkb.a")

add_library("Xext-static" STATIC IMPORTED)
set_property(TARGET "Xext-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/Xext/.libs/libXext.a")

add_library("Xi-static" STATIC IMPORTED)
set_property(TARGET "Xi-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/Xi/.libs/libXi.a")

add_library("glx-static" STATIC IMPORTED)
set_property(TARGET "glx-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/glx/.libs/libglx.a")

add_library("glxdri-static" STATIC IMPORTED)
set_property(TARGET "glxdri-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/glx/.libs/libglxdri.a")

add_library("xfixes-static" STATIC IMPORTED)
set_property(TARGET "xfixes-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/xfixes/.libs/libxfixes.a")

add_library("damageext-static" STATIC IMPORTED)
set_property(TARGET "damageext-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/damageext/.libs/libdamageext.a")

add_library("damage-static" STATIC IMPORTED)
set_property(TARGET "damage-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/miext/damage/.libs/libdamage.a")

add_library("sync-static" STATIC IMPORTED)
set_property(TARGET "sync-static" PROPERTY IMPORTED_LOCATION "${XOBJBASE}/miext/sync/.libs/libsync.a")

set(${MODULE_PREFIX}_INCLUDES
	"${XSRCBASE}/include"
	"${XSRCBASE}/Xext"
	"${XSRCBASE}/composite"
	"${XSRCBASE}/damageext"
	"${XSRCBASE}/xfixes"
	"${XSRCBASE}/Xi"
	"${XSRCBASE}/mi"
	"${XSRCBASE}/miext/damage"
	"${XSRCBASE}/miext/sync"
	"${XSRCBASE}/render"
	"${XSRCBASE}/randr"
	"${XSRCBASE}/fb"
	"${XSRCBASE}/hw/xfree86/common"
	"${XOBJBASE}/include")

include_directories(${${MODULE_PREFIX}_INCLUDES})

set(${MODULE_PREFIX}_DEFINES "-DHAVE_XORG_CONFIG_H -DXF86PM")

add_definitions(${${MODULE_PREFIX}_DEFINES})

add_executable(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

add_dependencies(${MODULE_NAME} xorg-server)

set(${MODULE_PREFIX}_LIBS z m freetype rt crypto dl ${CMAKE_THREAD_LIBS_INIT})

set_complex_link_libraries(VARIABLE ${MODULE_PREFIX}_LIBS
	MONOLITHIC ${MONOLITHIC_BUILD}
	MODULE winpr
	MODULES winpr-thread winpr-synch winpr-pipe winpr-input winpr-utils)

list(APPEND ${MODULE_PREFIX}_LIBS freerds-module-connector)

set(${MODULE_PREFIX}_STATIC_LIBS
	composite-static dbe-static dix-static main-static fb-static mi-static os-static randr-static record-static
	render-static xkb-static Xext-static Xi-static glx-static glxdri-static xfixes-static damageext-static damage-static sync-static)

list(APPEND ${MODULE_PREFIX}_LIBS
	${XFONT_LIBRARIES} ${XAU_LIBRARIES} ${XDMCP_LIBRARIES}
	${XDAMAGE_LIBRARIES} ${PIXMAN_LIBRARIES} ${GL_LIBRARIES})

merge_static_libs(combined ${${MODULE_PREFIX}_STATIC_LIBS})

target_link_libraries(${MODULE_NAME} combined ${${MODULE_PREFIX}_STATIC_LIBS} ${${MODULE_PREFIX}_LIBS})

if (ADDITIONAL_REQUIRED_LIBS)
	message(STATUS "Linking in additional libs")
	target_link_libraries(${MODULE_NAME} ${ADDITIONAL_REQUIRED_LIBS})
endif()

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

