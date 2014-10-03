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

set(FDSAPI_SRCS
	fdsapi/FDSApiHandler.cpp
	fdsapi/FDSApiServer.cpp
	fdsapi/FDSApiMessages.cpp)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(ICP_SOURCES 
	call/Call.cpp
	call/CallIn.cpp
	call/CallOut.cpp
	call/CallFactory.cpp
	call/CallInIsVCAllowed.cpp
	call/CallInPing.cpp
	call/CallOutPing.cpp
	call/CallInLogonUser.cpp
	call/CallInDisconnectUserSession.cpp
	call/CallInLogOffUserSession.cpp
	call/CallOutFdsApiVirtualChannelOpen.cpp
	call/CallInAuthenticateUser.cpp
	call/CallOutSwitchTo.cpp
	call/CallOutLogOffUserSession.cpp
	call/CallInEndSession.cpp
	call/TaskEndSession.cpp
	call/TaskSwitchTo.cpp	
	pbRPC/RpcEngine.cpp)

set(${MODULE_PREFIX}_SRCS
	main.cpp 
	appcontext/ApplicationContext.cpp 
	session/SessionStore.cpp
	session/Session.cpp
	session/ConnectionStore.cpp
	session/Connection.cpp
	session/TaskSessionTimeout.cpp
	module/ModuleManager.cpp
	module/Module.cpp
	module/AuthModule.cpp
	module/CallBacks.cpp
	module/TaskModuleShutdown.cpp
	config/PropertyManager.cpp
	config/PropertyCWrapper.cpp
	utils/StringHelpers.cpp
	task/Executor.cpp
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
	
target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS} ${PROTOBUF_LIBRARY})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})


