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

set(MODULE_NAME "${XRDP_NG_MODULE}-sesman")
set(MODULE_PREFIX "FREERDP_XRDP_SESMAN")

set(PAM_FEATURE_TYPE "REQUIRED")
set(PAM_FEATURE_PURPOSE "authentication")
set(PAM_FEATURE_DESCRIPTION "user authentication")

find_feature(PAM ${PAM_FEATURE_TYPE} ${PAM_FEATURE_PURPOSE} ${PAM_FEATURE_DESCRIPTION})

include_directories(${PAM_INCLUDE_DIR})

include_directories("../common")
include_directories("libscp")

set(${MODULE_PREFIX}_HEADERS
	sesman.h
	access.h
	auth.h
	cfg.h
	env.h
	lock.h
	scp.h
	scp_v0.h
	scp_v1.h
	scp_v1_mng.h
	session.h
	sig.h
	thread.h)

set(${MODULE_PREFIX}_SRCS
	scp.c
	scp_v0.c
	scp_v1.c
	scp_v1_mng.c
	sesman.c
	session.c
	sig.c
	thread.c
	lock.c
	access.c
	config.c
	env.c
	verify_user_pam.c
	${${MODULE_PREFIX}_HEADERS})

add_executable(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

add_subdirectory(libscp)
add_subdirectory(sessvc)
add_subdirectory(chansrv)

set(${MODULE_PREFIX}_LIBS
	${PAM_LIBRARY})

set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${XRDP_NG_MODULE}-common)
set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS} ${XRDP_NG_MODULE}-libscp)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_SBINDIR})

install(FILES "sesman.ini" DESTINATION ${XRDP_CFG_PATH})
install(PROGRAMS "startwm.sh" DESTINATION ${XRDP_CFG_PATH})

