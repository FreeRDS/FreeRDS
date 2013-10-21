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

add_subdirectory(module)

set(MODULE_PREFIX "FREERDS_SESSION_MANAGER")
set(MODULE_NAME "freerds-session-manager")

set(FREERDS_SESSION_MANAGER_VERSION_MAJOR "0")
set(FREERDS_SESSION_MANAGER_VERSION_MINOR "1")
set(FREERDS_SESSION_MANAGER_VERSION_REVISION "0")
set(FREERDS_SESSION_MANAGER_VERSION "${FREERDS_SESSION_MANAGER_VERSION_MAJOR}.${FREERDS_SESSION_MANAGER_VERSION_MINOR}")
set(FREERDS_SESSION_MANAGER_VERSION_FULL "${FREERDS_SESSION_MANAGER_VERSION}.${FREERDS_SESSION_MANAGER_VERSION_REVISION}")

get_directory_property(INCLUDE_DIRECTORIES INCLUDE_DIRECTORIES)
list(REMOVE_ITEM INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}")
set_directory_properties(PROPERTIES INCLUDE_DIRECTORIES "${INCLUDE_DIRECTORIES}")
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

find_feature(Protobuf REQUIRED "RPC" "Remote Procedure Call")

include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp)

set(${MODULE_PREFIX}_SRCS
	main.cpp 
	common/appcontext/ApplicationContext.cpp 
	common/session/SessionStore.cpp
	common/session/Session.cpp
	common/call/Call.cpp
	common/call/CallIn.cpp
	common/call/CallOut.cpp
	common/call/CallFactory.cpp
	common/call/CallIsVCAllowed.cpp
	common/pbRPC/RpcEngine.cpp
	gen-cpp/pbRPC.pb.cc
	gen-cpp/ICP.pb.cc
	common/module/ModuleManager.cpp
	common/module/Module.cpp
	common/config/PropertyManager.cpp
	common/config/PropertyCWrapper.cpp)
 
add_executable(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set_complex_link_libraries(VARIABLE ${MODULE_PREFIX}_LIBS
	MONOLITHIC ${MONOLITHIC_BUILD}
	MODULE winpr
	MODULES winpr-crt winpr-utils winpr-synch winpr-error winpr-file winpr-pipe winpr-library)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS} ${PROTOBUF_LIBRARY})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

