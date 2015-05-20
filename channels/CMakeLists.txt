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

project(freerds-channels CXX)

set(MODULE_PREFIX "FREERDS_CHANNELS")
set(MODULE_NAME "freerds-channels")

if(UNIX AND NOT APPLE)
	set(FUSE_FEATURE_TYPE "RECOMMENDED")
	set(FUSE_FEATURE_PURPOSE "FUSE")
	set(FUSE_FEATURE_DESCRIPTION "FUSE library")
	find_feature(FUSE ${FUSE_FEATURE_TYPE} ${FUSE_FEATURE_PURPOSE} ${FUSE_FEATURE_DESCRIPTION})
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(${MODULE_PREFIX}_SRCS
	channel_server.c
	channel_utils.c)
 
add_executable(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set(${MODULE_PREFIX}_LIBS winpr)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(cliprdr)

if(PULSE_FOUND)
	add_subdirectory(rdpsnd)
endif()

if(FUSE_FOUND)
	add_subdirectory(rdpdr)
endif()

