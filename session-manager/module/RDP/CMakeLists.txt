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

set(MODULE_NAME "freerds-module-rdp")
set(MODULE_PREFIX "FREERDS_MODULE_RDP")

set(${MODULE_PREFIX}_SRCS
	rdp_module.c
	rdp_module.h)

add_library(${MODULE_NAME} SHARED ${${MODULE_PREFIX}_SRCS})

set(${MODULE_PREFIX}_LIBS "")
list(APPEND ${MODULE_PREFIX}_LIBS freerds-module-connector)

set_complex_link_libraries(VARIABLE ${MODULE_PREFIX}_LIBS
	MONOLITHIC ${MONOLITHIC_BUILD}
	MODULE winpr
	MODULES winpr-crt winpr-synch winpr-thread winpr-environment winpr-utils winpr-utils)

list(APPEND ${MODULE_PREFIX}_LIBS} freerds-module-connector)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_subdirectory(service)
