
cmake_minimum_required(VERSION 2.8)

project(X11rdp)

include(GNUInstallDirs)

#    Copyright (C) 2012 Modelon AB

#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the BSD style license.

#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    FMILIB_License.txt file for more details.

#    You should have received a copy of the FMILIB_License.txt file
#    along with this program. If not, contact Modelon AB <http://www.modelon.com>.

# Merge_static_libs(outlib lib1 lib2 ... libn) merges a number of static
# libs into a single static library
function(merge_static_libs outlib)
	set(libs ${ARGV})
	list(REMOVE_AT libs 0)
# Create a dummy file that the target will depend on
	set(dummyfile ${CMAKE_CURRENT_BINARY_DIR}/${outlib}_dummy.c)
	file(WRITE ${dummyfile} "const char * dummy = \"${dummyfile}\";")
	
	add_library(${outlib} STATIC ${dummyfile})

	if("${CMAKE_CFG_INTDIR}" STREQUAL ".")
		set(multiconfig FALSE)
	else()
		set(multiconfig TRUE)
	endif()
	
# First get the file names of the libraries to be merged	
	foreach(lib ${libs})
		get_target_property(libtype ${lib} TYPE)
		if(NOT libtype STREQUAL "STATIC_LIBRARY")
			message(FATAL_ERROR "Merge_static_libs can only process static libraries")
		endif()
		if(multiconfig)
			foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
				get_target_property("libfile_${CONFIG_TYPE}" ${lib} "LOCATION_${CONFIG_TYPE}")
				list(APPEND libfiles_${CONFIG_TYPE} ${libfile_${CONFIG_TYPE}})
			endforeach()
		else()
			get_target_property(libfile ${lib} LOCATION)
			list(APPEND libfiles "${libfile}")
		endif(multiconfig)
	endforeach()
	#message(STATUS "will be merging ${libfiles}")
# Just to be sure: cleanup from duplicates
	if(multiconfig)	
		foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
			list(REMOVE_DUPLICATES libfiles_${CONFIG_TYPE})
			set(libfiles ${libfiles} ${libfiles_${CONFIG_TYPE}})
		endforeach()
	endif()
	list(REMOVE_DUPLICATES libfiles)

# Now the easy part for MSVC and for MAC
  if(MSVC)
    # lib.exe does the merging of libraries just need to conver the list into string
	foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
		set(flags "")
		foreach(lib ${libfiles_${CONFIG_TYPE}})
			set(flags "${flags} ${lib}")
		endforeach()
		string(TOUPPER "STATIC_LIBRARY_FLAGS_${CONFIG_TYPE}" PROPNAME)
		set_target_properties(${outlib} PROPERTIES ${PROPNAME} "${flags}")
	endforeach()
	
  elseif(APPLE)
    # Use OSX's libtool to merge archives
	if(multiconfig)
		message(FATAL_ERROR "Multiple configurations are not supported")
	endif()
	get_target_property(outfile ${outlib} LOCATION)  
	add_custom_command(TARGET ${outlib} POST_BUILD
		COMMAND rm ${outfile}
		COMMAND /usr/bin/libtool -static -o ${outfile} 
		${libfiles}
	)
  else() 
  # general UNIX - need to "ar -x" and then "ar -ru"
	if(multiconfig)
		message(FATAL_ERROR "Multiple configurations are not supported")
	endif()
	get_target_property(outfile ${outlib} LOCATION)
	message(STATUS "outfile location is ${outfile}")
	foreach(lib ${libfiles})
# objlistfile will contain the list of object files for the library
		set(objlistfile ${lib}.objlist)
		set(objdir ${lib}.objdir)
		set(objlistcmake  ${objlistfile}.cmake)
# we only need to extract files once 
		if(${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/cmake.check_cache IS_NEWER_THAN ${objlistcmake})
#---------------------------------			
			FILE(WRITE ${objlistcmake}
"# Extract object files from the library
message(STATUS \"Extracting object files from ${lib}\")
EXECUTE_PROCESS(COMMAND ${CMAKE_AR} -x ${lib}                
		WORKING_DIRECTORY ${objdir})
# save the list of object files
EXECUTE_PROCESS(COMMAND ls . 
				OUTPUT_FILE ${objlistfile}
		WORKING_DIRECTORY ${objdir})")
#---------------------------------					
			file(MAKE_DIRECTORY ${objdir})
			add_custom_command(
				OUTPUT ${objlistfile}
				COMMAND ${CMAKE_COMMAND} -P ${objlistcmake}
				DEPENDS ${lib})
		endif()
		list(APPEND extrafiles "${objlistfile}")
		# relative path is needed by ar under MSYS
		file(RELATIVE_PATH objlistfilerpath ${objdir} ${objlistfile})
		add_custom_command(TARGET ${outlib} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E echo "Running: ${CMAKE_AR} ru ${outfile} @${objlistfilerpath}"
			COMMAND ${CMAKE_AR} ru "${outfile}" @"${objlistfilerpath}"
			WORKING_DIRECTORY ${objdir})		
	endforeach()
	add_custom_command(TARGET ${outlib} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E echo "Running: ${CMAKE_RANLIB} ${outfile}"
			COMMAND ${CMAKE_RANLIB} ${outfile})
  endif()
  file(WRITE ${dummyfile}.base "const char* ${outlib}_sublibs=\"${libs}\";")
  add_custom_command( 
		OUTPUT  ${dummyfile}
		COMMAND ${CMAKE_COMMAND}  -E copy ${dummyfile}.base ${dummyfile}
		DEPENDS ${libs} ${extrafiles})

endfunction()

# X11rdp

set(MODULE_NAME "X11rdp")
set(MODULE_PREFIX "X11RDP")

set(XSRCBASE_RELATIVE "../xorg-server")
set(XSRCBASE "${CMAKE_CURRENT_SOURCE_DIR}/${XSRCBASE_RELATIVE}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -O2 -Wall -fno-strength-reduce")

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)

find_package(Threads REQUIRED)

include(FindPackageHandleStandardArgs)

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

# pixman

find_path(PIXMAN_INCLUDE_DIR NAMES pixman.h PATH_SUFFIXES pixman-1)

find_library(PIXMAN_LIBRARY NAMES pixman-1)

find_package_handle_standard_args(pixman-1 DEFAULT_MSG PIXMAN_LIBRARY PIXMAN_INCLUDE_DIR)

if(PIXMAN-1_FOUND)
	set(PIXMAN_LIBRARIES ${PIXMAN_LIBRARY})
	set(PIXMAN_INCLUDE_DIRS ${PIXMAN_INCLUDE_DIR})
endif()

mark_as_advanced(PIXMAN_INCLUDE_DIR PIXMAN_LIBRARY)

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

set(${MODULE_PREFIX}_SRCS ${${MODULE_PREFIX}_SRCS}
	${XSRCBASE_RELATIVE}/mi/miinitext.c
	${XSRCBASE_RELATIVE}/fb/fbcmap_mi.c)

add_library("composite-static" STATIC IMPORTED)
set_property(TARGET "composite-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/composite/.libs/libcomposite.a")

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

add_library("glxdri-static" STATIC IMPORTED)
set_property(TARGET "glxdri-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/glx/.libs/libglxdri.a")

add_library("xfixes-static" STATIC IMPORTED)
set_property(TARGET "xfixes-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/xfixes/.libs/libxfixes.a")

add_library("damageext-static" STATIC IMPORTED)
set_property(TARGET "damageext-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/damageext/.libs/libdamageext.a")

add_library("damage-static" STATIC IMPORTED)
set_property(TARGET "damage-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/miext/damage/.libs/libdamage.a")

add_library("sync-static" STATIC IMPORTED)
set_property(TARGET "sync-static" PROPERTY IMPORTED_LOCATION "${XSRCBASE}/miext/sync/.libs/libsync.a")

set(${MODULE_PREFIX}_INCLUDES
	"../../common"
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
	"${XSRCBASE}/hw/xfree86/common")

include_directories(${${MODULE_PREFIX}_INCLUDES})

set(${MODULE_PREFIX}_DEFINES "-DHAVE_XORG_CONFIG_H -DXF86PM")

add_definitions(${${MODULE_PREFIX}_DEFINES})

add_executable(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set(${MODULE_PREFIX}_LIBS z m freetype rt crypto dl ${CMAKE_THREAD_LIBS_INIT})

list(APPEND ${MODULE_PREFIX}_LIBS
	${XFONT_LIBRARIES} ${XAU_LIBRARIES} ${XDMCP_LIBRARIES}
	${XDAMAGE_LIBRARIES} ${PIXMAN_LIBRARIES} ${GL_LIBRARIES})

set(${MODULE_PREFIX}_STATIC_LIBS
	composite-static dbe-static dix-static main-static fb-static mi-static os-static randr-static record-static
	render-static xkb-static Xext-static Xi-static glx-static glxdri-static xfixes-static damageext-static damage-static sync-static)

merge_static_libs(combined ${${MODULE_PREFIX}_STATIC_LIBS})

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS} combined ${${MODULE_PREFIX}_STATIC_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
