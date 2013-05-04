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

set(MODULE_NAME "xrdp-ng-lib")
set(MODULE_PREFIX "FREERDP_XRDP_LIB")

include_directories("../common")

set(${MODULE_PREFIX}_HEADERS
	libxrdp.h
	libxrdpinc.h)

set(${MODULE_PREFIX}_SRCS
	libxrdp.c
	xrdp_channel.c
	xrdp_iso.c
	xrdp_mcs.c
	xrdp_orders.c
	xrdp_rdp.c
	xrdp_sec.c
	xrdp_tcp.c
	xrdp_bitmap_compress.c
	xrdp_jpeg_compress.c
	xrdp_orders_rail.c
	xrdp_mppc_enc.c
	${${MODULE_PREFIX}_HEADERS})

add_library(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} xrdp-ng-common)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})

