# FreeRDS: FreeRDP Remote Desktop Services (RDS)
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

project(freerds-session-manager CXX)

set(MODULE_PREFIX "FREERDS_SESSION_MANAGER")
set(MODULE_NAME "freerds-session-manager")

find_feature(Protobuf REQUIRED "RPC" "Remote Procedure Call")

PROTOBUF_GENERATE_CPP(PBRPC_PROTOBUF_SRC PBRPC_PROTOBUF_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../protocols/protobuf/pbRPC.proto)
PROTOBUF_GENERATE_CPP(ICP_PROTOBUF_SRC ICP_PROTOBUF_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../protocols/protobuf/ICP.proto)
PROTOBUF_GENERATE_CPP(ICPS_PROTOBUF_SRC ICPS_PROTOBUF_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../protocols/protobuf/ICPS.proto)

if(WITH_FDSAPI)
	include(ThriftExternalProject)

	include_directories(SYSTEM ${THRIFT_INCLUDE_DIR})
	set(THRIFT_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gen/fdsapi)
	file(MAKE_DIRECTORY ${THRIFT_OUTPUT_DIR})
	include(ThriftUtils)

	THRIFT_GEN(THRIFT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../protocols/thrift/fdsapi.thrift)
	add_definitions(-DWITH_FDSAPI)
	set(FDSAPI_SRC common/fdsapi/FDSApiHandler.cpp common/fdsapi/FDSApiServer.cpp)
endif(WITH_FDSAPI)

find_package(Boost REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )

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
	common/pbRPC/RpcEngine.cpp
)

set(${MODULE_PREFIX}_SRCS
	main.cpp 
	common/appcontext/ApplicationContext.cpp 
	common/session/SessionStore.cpp
	common/session/Session.cpp
	common/session/ConnectionStore.cpp
	common/session/Connection.cpp
	common/module/ModuleManager.cpp
	common/module/Module.cpp
	common/module/AuthModule.cpp
	common/module/CallBacks.cpp
	common/config/PropertyManager.cpp
	common/config/PropertyCWrapper.cpp
	common/utils/StringHelpers.cpp
	common/task/Executor.cpp
	${ICP_SOURCES}
	${FDSAPI_SRC}
	${PBRPC_PROTOBUF_SRC}
	${PBRPC_PROTOBUF_HEADERS}
	${ICP_PROTOBUF_SRC}
	${ICP_PROTOBUF_HEADERS}
	${ICPS_PROTOBUF_SRC}
	${ICPS_PROTOBUF_HEADERS}
	${THRIFT_FILES}
	)
 
add_executable(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})
add_dependencies(${MODULE_NAME} thrift)

set(${MODULE_PREFIX}_LIBS "")

set_complex_link_libraries(VARIABLE ${MODULE_PREFIX}_LIBS
	MONOLITHIC ${MONOLITHIC_BUILD}
	MODULE winpr
	MODULES winpr-crt winpr-utils winpr-synch winpr-error winpr-file winpr-path winpr-pipe winpr-library winpr-sspicli)

list(APPEND ${MODULE_PREFIX}_LIBS ${PROTOBUF_LIBRARY})
	
target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS} ${PROTOBUF_LIBRARY} ${THRIFT_STATIC_LIB})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(module)

if (WITH_FDSAPI)
add_subdirectory(fdsapi)
endif()
