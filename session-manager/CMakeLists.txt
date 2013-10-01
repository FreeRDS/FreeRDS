# FreeRemoteDesktopService Session Manager
# cmake script
#
# Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
# Copyright 2013 Bernhard Miklautz <bmiklautz@thinstuff.com>
# Copyright 2013 Thinstuff Technologies GmbH
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

project(freerds-session-manager CXX)

if(FREERDP_VERSION)
	set(FREERDP_BUILD ON)
endif()

set(FRDS_SM_VERSION_MAJOR "0")
set(FRDS_SM_VERSION_MINOR "1")
set(FRDS_SM_VERSION_REVISION "0")
set(FRDS_SM_VERSION "${FRDS_SM_VERSION_MAJOR}.${FRDS_SM_VERSION_MINOR}")
set(FRDS_SM_VERSION_FULL "${FRDS_SM_VERSION}.${FRDS_SM_VERSION_REVISION}")

set(CMAKE_COLOR_MAKEFILE ON)

include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckStructHasMember)
include(FindPkgConfig)
include(TestBigEndian)

if(NOT FREERDP_BUILD)
	set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/)
endif()

include(CheckCmakeCompat)

include(FindFeature)
include(AutoVersioning)
include(ConfigOptions)
include(ComplexLibrary)
include(FeatureSummary)
include(CheckCCompilerFlag)
include(GNUInstallDirsWrapper)

set(MODULE_PREFIX "SESSION_MANAGER")
set(MODULE_NAME "session-manager")


get_directory_property(INCLUDE_DIRECTORIES INCLUDE_DIRECTORIES)
list(REMOVE_ITEM INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}")
set_directory_properties(PROPERTIES INCLUDE_DIRECTORIES "${INCLUDE_DIRECTORIES}")
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

add_definitions("-DHAVE_CONFIG_H")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set_complex_link_libraries(VARIABLE ${MODULE_PREFIX}_LIBS
	MONOLITHIC ${MONOLITHIC_BUILD}
	MODULE session-manager
	MODULES winpr-crt winpr-synch winpr-error winpr-file winpr-pipe)


set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}:\$ORIGIN/..")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()


include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common
					${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp) 
 
set(${MODULE_PREFIX}_SRCS
	main.cpp 
	common/appcontext/ApplicationContext.cpp 
	common/session/SessionStore.cpp
	common/call/Call.cpp
	common/call/CallFactory.cpp
	common/call/CallIsVCAllowed.cpp
	common/pbRPC/RpcEngine.cpp
	gen-cpp/pbRPC.pb.cc
	gen-cpp/ICP.pb.cc)
 
add_executable(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})
				
target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS}  ${PROTOBUF_LIBRARY})
