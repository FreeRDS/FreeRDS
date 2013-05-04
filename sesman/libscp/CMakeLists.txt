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

set(MODULE_NAME "xrdp-ng-libscp")
set(MODULE_PREFIX "FREERDP_XRDP_LIBSCP")

include_directories("../../common")

include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${ZLIB_INCLUDE_DIRS})

set(${MODULE_PREFIX}_HEADERS
	libscp_connection.h
	libscp_commands.h
	libscp.h
	libscp_session.h
	libscp_types_mng.h
	libscp_v1c_mng.h
	libscp_vX.h
	libscp_commands_mng.h
	libscp_init.h
	libscp_tcp.h
	libscp_v0.h
	libscp_v1s.h
	libscp_lock.h
	libscp_types.h
	libscp_v1c.h
	libscp_v1s_mng.h)

set(${MODULE_PREFIX}_SRCS
	libscp_connection.c
	libscp_init.c
	libscp_lock.c
	libscp_session.c
	libscp_tcp.c
	libscp_v0.c
	libscp_v1c.c
	libscp_v1s.c
	libscp_v1c_mng.c
	libscp_v1s_mng.c
	libscp_vX.c
	${${MODULE_PREFIX}_HEADERS})

add_library(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set(${MODULE_PREFIX}_LIBS
	${CMAKE_THREAD_LIBS_INIT}
	${CMAKE_DL_LIBS})

if(${CMAKE_SYSTEM_NAME} MATCHES SunOS)
	set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} rt)
endif()

set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${ZLIB_LIBRARIES})
set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${OPENSSL_LIBRARIES})

set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} xrdp-ng-common)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})

