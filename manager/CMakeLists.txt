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

project(freerds-manager CXX)

set(MODULE_PREFIX "FREERDS_MANAGER")
set(MODULE_NAME "freerds-manager")

set(Boost_BOOST_NO_CMAKE ON)
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(${MODULE_PREFIX}_SRCS
	main.cpp 
	session/ApplicationContext.cpp 
	session/SessionStore.cpp
	session/Session.cpp
	session/ConnectionStore.cpp
	session/Connection.cpp
	session/TaskSessionTimeout.cpp
	session/PropertyManager.cpp
	session/PropertyCWrapper.cpp
	module/ModuleManager.cpp
	module/Module.cpp
	module/AuthModule.cpp
	module/CallBacks.cpp
	module/TaskModuleShutdown.cpp
	utils/StringHelpers.cpp
	task/Executor.cpp
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
	call/CallOutVirtualChannelOpen.cpp
	call/CallInAuthenticateUser.cpp
	call/CallOutSwitchTo.cpp
	call/CallOutLogOffUserSession.cpp
	call/CallInEndSession.cpp
	call/TaskEndSession.cpp
	call/TaskSwitchTo.cpp	
	call/RpcEngine.cpp
	fdsapi/FDSApiHandler.cpp
	fdsapi/FDSApiServer.cpp
	fdsapi/FDSApiMessages.cpp)
 
add_executable(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set(${MODULE_PREFIX}_LIBS freerds-rpc)

list(APPEND ${MODULE_PREFIX}_LIBS winpr)
	
target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
