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

set(MODULE_NAME "freerds-fdsapi")
set(MODULE_PREFIX "FREERDS_FDSAPI")

set(FDSAPI_GEN_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../gen")
set(FDSAPI_GEN_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../gen/fdsapi")

set(${MODULE_PREFIX}_GEN_SRCS
	${FDSAPI_GEN_SRC_DIR}/fdsapi_constants.cpp
	${FDSAPI_GEN_SRC_DIR}/fdsapi_types.cpp
	${FDSAPI_GEN_SRC_DIR}/fdsapi.cpp)

set(FDSAPI_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../common/fdsapi")

set_source_files_properties(${${MODULE_PREFIX}_GEN_SRCS} PROPERTIES GENERATED TRUE)

set(${MODULE_PREFIX}_SRCS
	fdsapi_thrift.cpp
	${FDSAPI_COMMON_DIR}/FDSApiMessages.cpp
	${${MODULE_PREFIX}_GEN_SRCS})

include_directories(
	${FDSAPI_GEN_INC_DIR}
	${FDSAPI_COMMON_DIR}
	${THRIFT_INCLUDE_DIR})

add_library(${MODULE_NAME} SHARED ${${MODULE_PREFIX}_SRCS})

set(${MODULE_PREFIX}_LIBS freerds-rpc)

list(APPEND ${MODULE_PREFIX}_LIBS ${THRIFT_STATIC_LIB})

list(APPEND ${MODULE_PREFIX}_LIBS winpr)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
