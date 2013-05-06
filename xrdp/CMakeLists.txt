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

set(MODULE_NAME "${XRDP_NG_MODULE}")
set(MODULE_PREFIX "FREERDP_XRDP")

include_directories("../common")
include_directories("../../../winpr/tools/makecert")

if(XRDP_NG_CORE)
	include_directories("../core")
else()
	include_directories("../libxrdp")
endif()

set(${MODULE_PREFIX}_SRCS
	funcs.c
	lang.c
	xrdp.c
	xrdp_bitmap.c
	xrdp_cache.c
	xrdp_font.c
	xrdp_listen.c
	xrdp_login_wnd.c
	xrdp_mm.c
	xrdp_painter.c
	xrdp_process.c
	xrdp_region.c
	xrdp_wm.c)

set(${MODULE_PREFIX}_CFG_DATA
	xrdp.ini)

set(${MODULE_PREFIX}_SHARE_DATA
	ad24b.bmp
	ad256.bmp
	xrdp24b.bmp
	xrdp256.bmp
	sans-10.fv1
	cursor0.cur
	cursor1.cur)

add_executable(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

if(XRDP_NG_CORE)
	add_definitions(-DWITH_XRDP_NG_CORE)
	set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${XRDP_NG_MODULE}-core)
else()
	set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${XRDP_NG_MODULE}-lib)
endif()

set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${XRDP_NG_MODULE}-common)

set_complex_link_libraries(VARIABLE ${MODULE_PREFIX}_LIBS
	MONOLITHIC ${MONOLITHIC_BUILD}
	MODULE winpr
	MODULES winpr-thread winpr-synch winpr-file winpr-path winpr-sysinfo winpr-utils winpr-crt)

set_complex_link_libraries(VARIABLE ${MODULE_PREFIX}_LIBS
	MONOLITHIC ${MONOLITHIC_BUILD}
	MODULE freerdp
	MODULES freerdp-core)

set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} winpr-makecert-tool)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_SBINDIR})

install(FILES ${${MODULE_PREFIX}_CFG_DATA} DESTINATION ${XRDP_CFG_PATH})
install(FILES ${${MODULE_PREFIX}_SHARE_DATA} DESTINATION ${XRDP_SHARE_PATH})

