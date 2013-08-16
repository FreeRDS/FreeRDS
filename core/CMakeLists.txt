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

set(MODULE_NAME "${XRDP_NG_MODULE}-core")
set(MODULE_PREFIX "FREERDP_XRDP_CORE")

include_directories("../common")

set(${MODULE_PREFIX}_SRCS
	core.c
	core.h)

add_library(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set_complex_link_libraries(VARIABLE ${MODULE_PREFIX}_LIBS
	MONOLITHIC ${MONOLITHIC_BUILD}
	MODULE freerdp
	MODULES freerdp-core freerdp-common freerdp-codec freerdp-primitives freerdp-utils freerdp-gdi freerdp-crypto freerdp-locale)

set_complex_link_libraries(VARIABLE ${MODULE_PREFIX}_LIBS
	MONOLITHIC ${MONOLITHIC_BUILD}
	MODULE winpr
	MODULES winpr-sspi winpr-crt winpr-utils winpr-input winpr-sysinfo)
	
list(APPEND ${MODULE_PREFIX}_LIBS ${PIXMAN_LIBRARIES})

set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} winpr-makecert-tool)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})

