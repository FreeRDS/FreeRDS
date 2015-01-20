# FreeRDS: FreeRDP Remote Desktop Services (RDS)
#
# Copyright 2014 Dell Software <Mike.McDonald@software.dell.com>
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

project(freerds-cliprdr-plugin CXX)

set(MODULE_PREFIX "FREERDS_CLIPRDR_PLUGIN")
set(MODULE_NAME "freerds-cliprdr-plugin")

include_directories(${X11_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(${MODULE_PREFIX}_SRCS
	cliprdr_plugin.c
	../channel_utils.c)
 
add_library(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set(${MODULE_PREFIX}_LIBS cliprdr-server winpr)

list(APPEND ${MODULE_PREFIX}_LIBS ${X11_LIBRARIES})

set(XFIXES_FEATURE_TYPE "RECOMMENDED")
set(XFIXES_FEATURE_PURPOSE "X11 xfixes extension")
set(XFIXES_FEATURE_DESCRIPTION "Useful additions to the X11 core protocol")

find_feature(Xfixes ${XFIXES_FEATURE_TYPE} ${XFIXES_FEATURE_PURPOSE} ${XFIXES_FEATURE_DESCRIPTION})

if(WITH_XFIXES)
	add_definitions(-DWITH_XFIXES)
	include_directories(${XFIXES_INCLUDE_DIRS})
	list(APPEND ${MODULE_PREFIX}_LIBS ${XFIXES_LIBRARIES})
endif()

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
