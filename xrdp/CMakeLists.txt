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

include_directories(".")
include_directories("../xup")
include_directories("../freerdp")
include_directories("../common")
include_directories("../../../winpr/tools/makecert")

include_directories("../core")

set(${MODULE_PREFIX}_SRCS
	funcs.c
	lang.c
	xrdp.c
	xrdp_auth.c
	xrdp_bitmap.c
	xrdp_listener.c
	xrdp_mm.c
	xrdp_module.c
	xrdp_pipeline.c
	xrdp_process.c
	xrdp_region.c
	xrdp_wm.c)

set(${MODULE_PREFIX}_CFG_DATA
	xrdp.ini)

add_executable(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${XRDP_NG_MODULE}-core)
set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${XRDP_NG_MODULE}-xup)
set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${XRDP_NG_MODULE}-freerdp)
set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${XRDP_NG_MODULE}-common)

set_complex_link_libraries(VARIABLE ${MODULE_PREFIX}_LIBS
	MONOLITHIC ${MONOLITHIC_BUILD}
	MODULE winpr
	MODULES winpr-thread winpr-synch winpr-file winpr-path winpr-sysinfo winpr-utils winpr-crt)

set_complex_link_libraries(VARIABLE ${MODULE_PREFIX}_LIBS
	MONOLITHIC ${MONOLITHIC_BUILD}
	MODULE freerdp
	MODULES freerdp-core)

list(APPEND ${MODULE_PREFIX}_LIBS ${PIXMAN_LIBRARIES})

list(APPEND ${MODULE_PREFIX}_LIBS winpr-makecert-tool)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_SBINDIR})

install(FILES ${${MODULE_PREFIX}_CFG_DATA} DESTINATION ${XRDP_CFG_PATH})

