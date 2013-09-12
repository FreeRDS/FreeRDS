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

set(PAM_FEATURE_TYPE "REQUIRED")
set(PAM_FEATURE_PURPOSE "authentication")
set(PAM_FEATURE_DESCRIPTION "user authentication")

find_feature(PAM ${PAM_FEATURE_TYPE} ${PAM_FEATURE_PURPOSE} ${PAM_FEATURE_DESCRIPTION})

include_directories(${PAM_INCLUDE_DIR})

include_directories(".")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../common")
include_directories("${CMAKE_SOURCE_DIR}/winpr/tools/makecert")

set(${MODULE_PREFIX}_SRCS
	xrdp.c
	xrdp_auth.c
	xrdp_core.c
	xrdp_core.h
	xrdp_channels.c
	xrdp_channels.h
	xrdp_listener.c
	xrdp_mm.c
	xrdp_pipeline.c
	xrdp_process.c
	xrdp_client_module.c
	xrdp_server_module.c)

set(${MODULE_PREFIX}_CFG_DATA
	xrdp.ini)

add_executable(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set(${MODULE_PREFIX}_LIBS ${PAM_LIBRARY})

set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${XRDP_NG_MODULE}-common)
set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${XRDP_NG_MODULE}-protocol)

set_complex_link_libraries(VARIABLE ${MODULE_PREFIX}_LIBS
	MONOLITHIC ${MONOLITHIC_BUILD}
	MODULE winpr
	MODULES winpr-thread winpr-synch winpr-file winpr-path winpr-sysinfo winpr-pipe winpr-utils winpr-crt)

set_complex_link_libraries(VARIABLE ${MODULE_PREFIX}_LIBS
	MONOLITHIC ${MONOLITHIC_BUILD}
	MODULE freerdp
	MODULES freerdp-core)
	
list(APPEND ${MODULE_PREFIX}_LIBS freerdp-server)

list(APPEND ${MODULE_PREFIX}_LIBS ${PIXMAN_LIBRARIES})

list(APPEND ${MODULE_PREFIX}_LIBS winpr-makecert-tool)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_SBINDIR})

install(FILES ${${MODULE_PREFIX}_CFG_DATA} DESTINATION ${XRDP_CFG_PATH})

