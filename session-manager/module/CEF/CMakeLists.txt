# FreeRDS: FreeRDP Remote Desktop Services (RDS)
#
# Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

set(MODULE_NAME "freerds-module-cef")
set(MODULE_PREFIX "FREERDS_MODULE_CEF")

find_path(CEF_INCLUDE_DIR
	NAMES cef_version.h
	PATHS /opt/cef/include)

find_library(CEF_DEBUG_LIBRARY
	NAMES cef
	PATHS /opt/cef/Debug)

find_library(CEF_RELEASE_LIBRARY
	NAMES cef
	PATHS /opt/cef/Release)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	set(CEF_LIBRARY ${CEF_DEBUG_LIBRARY})
else()
	set(CEF_LIBRARY ${CEF_RELEASE_LIBRARY})
endif()

find_package_handle_standard_args(CEF DEFAULT_MSG CEF_LIBRARY CEF_INCLUDE_DIR)

if(CEF_FOUND)
	set(CEF_LIBRARIES ${CEF_LIBRARY})
	set(CEF_INCLUDE_DIRS ${CEF_INCLUDE_DIR})
endif()

set(${MODULE_PREFIX}_SRCS
	cef_module.c
	cef_module.h)

add_library(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set_complex_link_libraries(VARIABLE ${MODULE_PREFIX}_LIBS
	MONOLITHIC ${MONOLITHIC_BUILD}
	MODULE winpr
	MODULES winpr-crt winpr-synch winpr-thread winpr-environment winpr-utils winpr-utils)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(CEF_FOUND)
	add_subdirectory(service)
endif()

