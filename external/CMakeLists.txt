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

add_subdirectory(msgpack)

include(ExternalProject)

set(EXTERNAL_PROJECTS_BASE "${CMAKE_CURRENT_BINARY_DIR}")
set_property(DIRECTORY PROPERTY "EP_BASE" ${EXTERNAL_PROJECTS_BASE})


#
# pulseaudio (used by RDPSND)
#

include(FindPulse)

message(STATUS "Using pulseaudio sources version ${PULSE_VERSION_MAJOR}.${PULSE_VERSION_MINOR}")

set(EXTERNAL_PROJECT "pulseaudio")
set(${EXTERNAL_PROJECT}_VERSION "${PULSE_VERSION}")
set(${EXTERNAL_PROJECT}_NAME "${EXTERNAL_PROJECT}-${${EXTERNAL_PROJECT}_VERSION}")
set(${EXTERNAL_PROJECT}_EXT ".tar.gz")
set(${EXTERNAL_PROJECT}_FILE "${${EXTERNAL_PROJECT}_NAME}${${EXTERNAL_PROJECT}_EXT}")
set(${EXTERNAL_PROJECT}_URL "http://freedesktop.org/software/pulseaudio/releases/${${EXTERNAL_PROJECT}_FILE}")
set(${EXTERNAL_PROJECT}_CONFIGURE "${EXTERNAL_PROJECTS_BASE}/Source/${EXTERNAL_PROJECT}/configure")
set(${EXTERNAL_PROJECT}_UPDATE "")
set(${EXTERNAL_PROJECT}_MAKE "")

message(STATUS "pulseaudio is in ${EXTERNAL_PROJECTS_BASE}/Source/${EXTERNAL_PROJECT}")

ExternalProject_Add(${EXTERNAL_PROJECT}
	URL ${${EXTERNAL_PROJECT}_URL}
	UPDATE_COMMAND ${${EXTERNAL_PROJECT}_UPDATE}
	CONFIGURE_COMMAND ${${EXTERNAL_PROJECT}_CONFIGURE}
	BUILD_IN_SOURCE 1
	INSTALL_COMMAND "")


#
# FUSE - Filesystem in User Space (used by RDPDR)
#

if(FALSE)

set(EXTERNAL_PROJECT "fuse")
set(${EXTERNAL_PROJECT}_VERSION "2.9.3")
set(${EXTERNAL_PROJECT}_NAME "${EXTERNAL_PROJECT}-${${EXTERNAL_PROJECT}_VERSION}")
set(${EXTERNAL_PROJECT}_EXT ".tar.gz")
set(${EXTERNAL_PROJECT}_FILE "${${EXTERNAL_PROJECT}_NAME}${${EXTERNAL_PROJECT}_EXT}")
set(${EXTERNAL_PROJECT}_URL "http://sourceforge.net/projects/fuse/files/fuse-2.X/2.9.3/${${EXTERNAL_PROJECT}_FILE}/download")
set(${EXTERNAL_PROJECT}_CONFIGURE "${EXTERNAL_PROJECTS_BASE}/Source/${EXTERNAL_PROJECT}/configure")
set(${EXTERNAL_PROJECT}_UPDATE "")
set(${EXTERNAL_PROJECT}_MAKE "")

message(STATUS "FUSE is in ${EXTERNAL_PROJECTS_BASE}/Source/${EXTERNAL_PROJECT}")

ExternalProject_Add(${EXTERNAL_PROJECT}
	URL ${${EXTERNAL_PROJECT}_URL}
	UPDATE_COMMAND ${${EXTERNAL_PROJECT}_UPDATE}
	CONFIGURE_COMMAND ${${EXTERNAL_PROJECT}_CONFIGURE}
	BUILD_IN_SOURCE 1
	INSTALL_COMMAND "")

endif()
