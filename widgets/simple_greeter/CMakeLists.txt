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

project(simple_greeter CXX)

set(MODULE_PREFIX "SIMPLE_GREETER")
set(MODULE_NAME "simple_greeter")

find_package(Qt4 REQUIRED)

set(${MODULE_PREFIX}_SOURCES
	simple_greeter.cpp
	)

set(${MODULE_PREFIX}_HEADERS
	simple_greeter.h
	)

set(${MODULE_PREFIX}_UI
	simple_greeter.ui
	)

set(${MODULE_PREFIX}_RESOURCES
	simple_greeter.qrc
	)

QT4_WRAP_CPP(${MODULE_PREFIX}_HEADERS_MOC ${${MODULE_PREFIX}_HEADERS})
QT4_WRAP_UI(${MODULE_PREFIX}_UI_HEADERS ${${MODULE_PREFIX}_UI})
QT4_ADD_RESOURCES(${MODULE_PREFIX}_RESOURCES_RCC ${${MODULE_PREFIX}_RESOURCES})

include(${QT_USE_FILE})

add_definitions(${QT_DEFINITIONS})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(${MODULE_NAME}
	${${MODULE_PREFIX}_SOURCES}
	${${MODULE_PREFIX}_HEADERS_MOC}
	${${MODULE_PREFIX}_UI_HEADERS}
	${${MODULE_PREFIX}_RESOURCES_RCC}
	)

set(${MODULE_PREFIX}_LIBS "")

list(APPEND ${MODULE_PREFIX}_LIBS winpr)

list(APPEND ${MODULE_PREFIX}_LIBS ${QT_LIBRARIES} freerds-fdsapi)
	
target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

