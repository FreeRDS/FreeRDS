# FreeRDS: FreeRDP Remote Desktop Services (RDS)
#
# Copyright 2013 Thincast Technologies GmbH
# Copyright 2013 Bernhard Miklautz <bernhard.miklautz@thincast.com>
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

set(MODULE_NAME "freerds-icp")
set(MODULE_PREFIX "FREERDS_ICP")

PROTOBUFC_GENERATE_C(ICP_PROTOC_SRC ICP_PROTOC_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/../../protocols/protobuf/ICP.proto)
PROTOBUFC_GENERATE_C(PBRPC_PROTOC_SRC PBRPC_PROTOC_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/../../protocols/protobuf/pbRPC.proto)

set(PBRPC_SRC
	pbrpc.c
	${PBRPC_PROTOC_HDRS}
	${PBRPC_PROTOC_SRC})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(${MODULE_PREFIX}_SRCS
	icp.c
	icp_client_stubs.c
	icp_server_stubs.c
	${ICP_PROTOC_SRC}
	${ICP_PROTOC_HDRS}
	${PBRPC_SRC})

add_library(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

list(APPEND ${MODULE_PREFIX}_LIBS winpr)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS} ${PROTOBUFC_LIBRARIES})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
