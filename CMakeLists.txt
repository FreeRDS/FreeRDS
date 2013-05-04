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

cmake_minimum_required(VERSION 2.8)

project(xrdp-ng C)

set(CMAKE_COLOR_MAKEFILE ON)

include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckStructHasMember)
include(FindPkgConfig)
include(TestBigEndian)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/)

include(CheckCmakeCompat)

include(FindFeature)
include(AutoVersioning)
include(ConfigOptions)
include(ComplexLibrary)
include(FeatureSummary)
include(CheckCCompilerFlag)
include(GNUInstallDirsWrapper)

set(XRDP_NG_VERSION_MAJOR "0")
set(XRDP_NG_VERSION_MINOR "1")
set(XRDP_NG_VERSION_REVISION "0")
set(XRDP_NG_VERSION "${XRDP_NG_VERSION_MAJOR}.${XRDP_NG_VERSION_MINOR}")
set(XRDP_NG_VERSION_FULL "${XRDP_NG_VERSION}.${XRDP_NG_VERSION_REVISION}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(SBINDIR "${CMAKE_INSTALL_PREFIX}/sbin")
set(SYSCONFDIR "${CMAKE_INSTALL_PREFIX}/etc")
set(DATADIR "${CMAKE_INSTALL_PREFIX}/share")
set(LOCALSTATEDIR "${CMAKE_INSTALL_PREFIX}/var")

set(XRDP_CFG_PATH "${SYSCONFDIR}/xrdp")
set(XRDP_SBIN_PATH "${SBINDIR}")
set(XRDP_SHARE_PATH "${DATADIR}/xrdp")
set(XRDP_PID_PATH "${LOCALSTATEDIR}/run")
set(XRDP_LOG_PATH "${LOCALSTATEDIR}/log")

add_definitions("-DHAVE_CONFIG_H")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT DEFINED BUILD_SHARED_LIBS)
	set(BUILD_SHARED_LIBS ON)
endif()

add_subdirectory(common)
add_subdirectory(libxrdp)
add_subdirectory(xrdp)
add_subdirectory(sesman)
add_subdirectory(rdp)
add_subdirectory(xup)
add_subdirectory(mc)
add_subdirectory(keygen)

