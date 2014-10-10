# FreeRDS: FreeRDP Remote Desktop Services (RDS)
#
# Copyright 2013-2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

project(FreeRDS C)

if(FREERDP_VERSION)
	set(FREERDP_BUILD ON)
endif()

set(CMAKE_COLOR_MAKEFILE ON)

include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckStructHasMember)
include(FindPkgConfig)
include(TestBigEndian)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
if(NOT FREERDP_BUILD)
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/")
endif()

include(CheckCmakeCompat)

include(FindFeature)
include(AutoVersioning)
include(ConfigOptions)
include(ComplexLibrary)
include(FeatureSummary)
include(CheckCCompilerFlag)
include(GNUInstallDirsWrapper)
include(CMakePackageConfigHelpers)

set(FREERDS_VERSION_MAJOR "0")
set(FREERDS_VERSION_MINOR "1")
set(FREERDS_VERSION_REVISION "0")
set(FREERDS_VERSION "${FREERDS_VERSION_MAJOR}.${FREERDS_VERSION_MINOR}")
set(FREERDS_VERSION_FULL "${FREERDS_VERSION}.${FREERDS_VERSION_REVISION}")

set(EXTERNAL_PROJECTS_BASE "${CMAKE_CURRENT_BINARY_DIR}/external")
set_property(DIRECTORY PROPERTY "EP_BASE" ${EXTERNAL_PROJECTS_BASE})

get_directory_property(INCLUDE_DIRECTORIES INCLUDE_DIRECTORIES)
list(REMOVE_ITEM INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}")
set_directory_properties(PROPERTIES INCLUDE_DIRECTORIES "${INCLUDE_DIRECTORIES}")
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
set(SBINDIR "${CMAKE_INSTALL_PREFIX}/sbin")
set(SYSCONFDIR "${CMAKE_INSTALL_PREFIX}/etc")
set(DATADIR "${CMAKE_INSTALL_PREFIX}/share")
set(LOCALSTATEDIR "${CMAKE_INSTALL_PREFIX}/var")

set(FREERDS_CFG_PATH "${SYSCONFDIR}/freerds")
set(FREERDS_BIN_PATH "${BINDIR}")
set(FREERDS_SBIN_PATH "${SBINDIR}")
set(FREERDS_SHARE_PATH "${DATADIR}/freerds")
set(FREERDS_VAR_PATH "${LOCALSTATEDIR}")
set(FREERDS_PID_PATH "${LOCALSTATEDIR}/run")
set(FREERDS_LOG_PATH "${LOCALSTATEDIR}/log")
set(FREERDS_LIB_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

add_definitions("-DHAVE_CONFIG_H")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}:\$ORIGIN/..")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT DEFINED BUILD_SHARED_LIBS)
	set(BUILD_SHARED_LIBS ON)
endif()

add_subdirectory(external)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/msgpack/src)

add_subdirectory(include)
add_subdirectory(freerds)
add_subdirectory(scripts)
add_subdirectory(manager)
add_subdirectory(fdsapi)
add_subdirectory(module)

if(${CMAKE_VERSION} VERSION_GREATER "2.8.10")

	export(PACKAGE freerds)

	set(FREERDS_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/FreeRDS")

	set(FREERDS_INCLUDE_DIR "include")

	configure_package_config_file(FreeRDSConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/FreeRDSConfig.cmake
		INSTALL_DESTINATION ${FREERDS_CMAKE_INSTALL_DIR}
		PATH_VARS FREERDS_INCLUDE_DIR)

	write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/FreeRDSConfigVersion.cmake
		VERSION ${FREERDS_VERSION} COMPATIBILITY SameMajorVersion)

	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/FreeRDSConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/FreeRDSConfigVersion.cmake
		DESTINATION ${FREERDS_CMAKE_INSTALL_DIR})

	install(EXPORT FreeRDSTargets DESTINATION ${FREERDS_CMAKE_INSTALL_DIR})

endif()

if(NOT MSVC)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/freerds-backend.pc.in ${CMAKE_CURRENT_BINARY_DIR}/freerds-backend.pc @ONLY)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/freerds-backend.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()
