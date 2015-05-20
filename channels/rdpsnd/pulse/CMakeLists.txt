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

project(module-freerds-sink CXX)

set(MODULE_PREFIX "MODULE_FREERDS_SINK")
set(MODULE_NAME "module-freerds-sink")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include_directories(.. ../..)

set(${MODULE_PREFIX}_PULSEAUDIO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../../../external/Source/pulseaudio")

get_directory_property(INCLUDE_DIRECTORIES INCLUDE_DIRECTORIES)
list(INSERT INCLUDE_DIRECTORIES 0 "${${MODULE_PREFIX}_PULSEAUDIO_SRC}")
list(INSERT INCLUDE_DIRECTORIES 0 "${${MODULE_PREFIX}_PULSEAUDIO_SRC}/src")
set_directory_properties(PROPERTIES INCLUDE_DIRECTORIES "${INCLUDE_DIRECTORIES}")

include_directories(${${MODULE_PREFIX}_PULSEAUDIO_SRC})
include_directories(${${MODULE_PREFIX}_PULSEAUDIO_SRC}/src)

link_directories(${CMAKE_INSTALL_PREFIX}/lib)

set(${MODULE_PREFIX}_SRCS
	module-freerds-sink.c
	../rdpsnd_plugin_api.c
	../../channel_utils.c)
 
add_library(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set(${MODULE_PREFIX}_LIBS ${PULSE_LIBRARY} freerds-rpc winpr)

set_target_properties(${MODULE_NAME} PROPERTIES PREFIX "")

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(TARGET_FILE ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${MODULE_NAME}.so)
set(LINK_NAME /usr/${CMAKE_INSTALL_LIBDIR}/pulse-${PULSE_VERSION}/modules/${MODULE_NAME}.so)

message(STATUS "Creating symbolic link for pulseaudio sink")
message(STATUS "TARGET_FILE=${TARGET_FILE}")
message(STATUS "LINK_NAME=${LINK_NAME}")

install(CODE "EXECUTE_PROCESS(COMMAND ln -s ${TARGET_FILE} ${LINK_NAME})")
