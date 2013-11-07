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

cmake_minimum_required(VERSION 2.8)
project(external-xorg-build)

include(GNUInstallDirs)
include(ExternalProject)

if (NOT FULL_X11_STACK)
	set(FULL_X11_STACK FALSE)
endif()

if (NOT XORG_TO_BUILD)
	set(XORG_TO_BUILD "X11R7.7")
endif()


set(EXTERNAL_PROJECTS_BASE "${CMAKE_CURRENT_BINARY_DIR}/external")
set_property(DIRECTORY PROPERTY "EP_BASE" ${EXTERNAL_PROJECTS_BASE})

execute_process(COMMAND "X" -version ERROR_VARIABLE XORG_SERVER_VERSION_OUTPUT)

STRING(REGEX MATCH "[0-9]+.[0-9]+.[0-9]+" XORG_SERVER_VERSION "${XORG_SERVER_VERSION_OUTPUT}")
if (NOT XORG_SERVER_VERSION)
	set(XORG_SERVER_VERSION "0.0.0")
endif()

if ((${XORG_SERVER_VERSION} VERSION_LESS "1.12.99.904") OR FULL_X11_STACK)
	message(STATUS "Installed X version is too old (${XORG_SERVER_VERSION}. Building complete xorg stack")
	message(STATUS "Using ${XORG_TO_BUILD} - overwrite by setting  -DXORG_TO_BUILD=X11Rx.y")
	add_subdirectory(${XORG_TO_BUILD})
else()
		if (EXISTS "/etc/debian_version")
			message("/etc/debian_version found - trying debian build")
			execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/debian_prepare_xorg.sh)
			add_custom_target(xorg-debian
												COMMAND cd external/xorg-server && ./debian/rules build)
			add_custom_target(all DEPENDS xorg-debian)
			message(NOTE: " run the following command after build: 
			cp ${CMAKE_CURRENT_SOURCE_DIR}/scripts/LocalConfigSettings.cmake ${CMAKE_CURRENT_SOURCE_DIR}/../rdp/")
		else()
			message(STATUS "Using xorg-server sources version ${XORG_SERVER_VERSION}")

			set(EXTERNAL_PROJECT "xorg-server")
			set(${EXTERNAL_PROJECT}_VERSION "${XORG_SERVER_VERSION}")
			set(${EXTERNAL_PROJECT}_NAME "${EXTERNAL_PROJECT}-${${EXTERNAL_PROJECT}_VERSION}")
			set(${EXTERNAL_PROJECT}_EXT ".tar.bz2")
			set(${EXTERNAL_PROJECT}_FILE "${${EXTERNAL_PROJECT}_NAME}${${EXTERNAL_PROJECT}_EXT}")
			set(${EXTERNAL_PROJECT}_URL "http://xorg.freedesktop.org/releases/individual/xserver/${${EXTERNAL_PROJECT}_FILE}")
			set(${EXTERNAL_PROJECT}_CONFIGURE "--prefix=/usr;--with-sha1=libcrypto;--disable-dpms")
			set(${EXTERNAL_PROJECT}_UPDATE "")
			set(${EXTERNAL_PROJECT}_MAKE "")

			ExternalProject_Add(${EXTERNAL_PROJECT}
				URL ${${EXTERNAL_PROJECT}_URL}
				UPDATE_COMMAND ${${EXTERNAL_PROJECT}_UPDATE}
				CONFIGURE_COMMAND "${EXTERNAL_PROJECTS_BASE}/Source/${EXTERNAL_PROJECT}/configure" ${${EXTERNAL_PROJECT}_CONFIGURE}
				BUILD_IN_SOURCE 1
				INSTALL_COMMAND "")
	endif()
endif()
