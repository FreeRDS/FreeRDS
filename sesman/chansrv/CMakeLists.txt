# FreeRDP X11 Server Next Generation
# xrdp-ng cmake build script
#
# Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

set(MODULE_NAME "${XRDP_NG_MODULE}-chansrv")
set(MODULE_PREFIX "FREERDP_XRDP_CHANSRV")

set(X11_FEATURE_TYPE "REQUIRED")
set(X11_FEATURE_PURPOSE "X11")
set(X11_FEATURE_DESCRIPTION "X11 client and server")

set(XFIXES_FEATURE_TYPE "RECOMMENDED")
set(XFIXES_FEATURE_PURPOSE "X11 region")
set(XFIXES_FEATURE_DESCRIPTION "X11 region fix extension")

find_feature(X11 ${X11_FEATURE_TYPE} ${X11_FEATURE_PURPOSE} ${X11_FEATURE_DESCRIPTION})
find_feature(Xfixes ${XFIXES_FEATURE_TYPE} ${XFIXES_FEATURE_PURPOSE} ${XFIXES_FEATURE_DESCRIPTION})

include_directories(${X11_INCLUDE_DIRS})
include_directories(${XFIXES_INCLUDE_DIRS})

include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${ZLIB_INCLUDE_DIRS})

include_directories("../../common")

set(${MODULE_PREFIX}_HEADERS
	chansrv.h
	clipboard.h
	devredir.h
	sound.h)

set(${MODULE_PREFIX}_SRCS
	chansrv.c
	sound.c
	clipboard.c
	clipboard_file.c
	devredir.c
	rail.c
	xcommon.c
	drdynvc.c
	chansrv_fuse.c
	${${MODULE_PREFIX}_HEADERS})

add_executable(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set(${MODULE_PREFIX}_LIBS
	${CMAKE_THREAD_LIBS_INIT}
	${CMAKE_DL_LIBS})

if(${CMAKE_SYSTEM_NAME} MATCHES SunOS)
	set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} rt)
endif()

set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${X11_LIBRARIES})
set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${XFIXES_LIBRARIES})

set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${ZLIB_LIBRARIES})
set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${OPENSSL_LIBRARIES})

set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${XRDP_NG_MODULE}-common)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_SBINDIR})

