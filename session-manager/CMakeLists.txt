# FreeRDS: FreeRDP Remote Desktop Services (RDS)
#
# Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
# Copyright 2013 Bernhard Miklautz <bernhard.miklautz@thincast.com>
# Copyright 2013 Thincast Technologies GmbH
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

project(freerds-session-manager CXX)

set(MODULE_PREFIX "FREERDS_SESSION_MANAGER")
set(MODULE_NAME "freerds-session-manager")

find_feature(Protobuf REQUIRED "RPC" "Remote Procedure Call")

PROTOBUF_GENERATE_CPP(PBRPC_PROTOBUF_SRC PBRPC_PROTOBUF_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../protocols/protobuf/pbRPC.proto)
PROTOBUF_GENERATE_CPP(ICP_PROTOBUF_SRC ICP_PROTOBUF_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../protocols/protobuf/ICP.proto)
PROTOBUF_GENERATE_CPP(ICPS_PROTOBUF_SRC ICPS_PROTOBUF_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../protocols/protobuf/ICPS.proto)

find_feature(Thrift REQUIRED "RPC" "Remote Procedure Call")
	
if(NOT THRIFT_FOUND)
	message(STATUS "You may need to manually download and install thrift:")
	message(STATUS "wget http://archive.apache.org/dist/thrift/0.9.0/thrift-0.9.0.tar.gz")
	message(STATUS "tar zxvf thrift-0.9.0.tar.gz && cd thrift-0.9.0")
	message(STATUS "./configure --without-python --without-java --without-c_glib --with-pic --without-csharp --without-haskell --without-go --without-d --without-qt4")
	message(STATUS "make && sudo make install")
endif()

set(FDSAPI_SRCS
	common/fdsapi/fdsapi.cpp
	common/fdsapi/fdsapi_types.cpp
	common/fdsapi/FDSApiHandler.cpp
	common/fdsapi/FDSApiServer.cpp
	common/fdsapi/FDSApiMessages.cpp)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/gen)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(ICP_SOURCES 
	common/call/Call.cpp
	common/call/CallIn.cpp
	common/call/CallOut.cpp
	common/call/CallFactory.cpp
	common/call/CallInIsVCAllowed.cpp
	common/call/CallInPing.cpp
	common/call/CallOutPing.cpp
	common/call/CallInLogonUser.cpp
	common/call/CallInDisconnectUserSession.cpp
	common/call/CallInLogOffUserSession.cpp
	common/call/CallOutFdsApiVirtualChannelOpen.cpp
	common/call/CallInAuthenticateUser.cpp
	common/call/CallOutSwitchTo.cpp
	common/call/CallOutLogOffUserSession.cpp
	common/call/CallInEndSession.cpp
	common/call/TaskEndSession.cpp
	common/call/TaskSwitchTo.cpp	
	common/pbRPC/RpcEngine.cpp)

set(${MODULE_PREFIX}_SRCS
	main.cpp 
	common/appcontext/ApplicationContext.cpp 
	common/session/SessionStore.cpp
	common/session/Session.cpp
	common/session/ConnectionStore.cpp
	common/session/Connection.cpp
	common/session/TaskSessionTimeout.cpp
	common/module/ModuleManager.cpp
	common/module/Module.cpp
	common/module/AuthModule.cpp
	common/module/CallBacks.cpp
	common/module/TaskModuleShutdown.cpp
	common/config/PropertyManager.cpp
	common/config/PropertyCWrapper.cpp
	common/utils/StringHelpers.cpp
	common/task/Executor.cpp
	${ICP_SOURCES}
	${FDSAPI_SRCS}
	${PBRPC_PROTOBUF_SRC}
	${PBRPC_PROTOBUF_HEADERS}
	${ICP_PROTOBUF_SRC}
	${ICP_PROTOBUF_HEADERS}
	${ICPS_PROTOBUF_SRC}
	${ICPS_PROTOBUF_HEADERS})
 
add_executable(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set(${MODULE_PREFIX}_LIBS freerds-rpc)

list(APPEND ${MODULE_PREFIX}_LIBS winpr)

list(APPEND ${MODULE_PREFIX}_LIBS ${PROTOBUF_LIBRARY})
	
target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS} ${PROTOBUF_LIBRARY} ${THRIFT_STATIC_LIB})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(module)

add_subdirectory(fdsapi)
