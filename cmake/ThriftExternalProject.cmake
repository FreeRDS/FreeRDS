# Build thrift as external project
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

include(ExternalProject)

set(EXTERNAL_PROJECT "thrift")

set(${EXTERNAL_PROJECT}_VERSION "0.9.0")
set(${EXTERNAL_PROJECT}_NAME "${EXTERNAL_PROJECT}-${${EXTERNAL_PROJECT}_VERSION}")
set(${EXTERNAL_PROJECT}_EXT ".tar.gz")
set(${EXTERNAL_PROJECT}_FILE "${${EXTERNAL_PROJECT}_NAME}${${EXTERNAL_PROJECT}_EXT}")
set(${EXTERNAL_PROJECT}_URL "http://archive.apache.org/dist/thrift/${${EXTERNAL_PROJECT}_VERSION}/${${EXTERNAL_PROJECT}_FILE}")
set(${EXTERNAL_PROJECT}_CONFIGURE --without-python --without-java --without-c_glib --with-pic --without-csharp --without-haskell --without-go --without-d --without-qt4
--prefix=${EXTERNAL_PROJECTS_BASE}/Install/${EXTERNAL_PROJECT})
set(${EXTERNAL_PROJECT}_UPDATE "")
set(${EXTERNAL_PROJECT}_MAKE "")

ExternalProject_Add(${EXTERNAL_PROJECT}
		URL ${${EXTERNAL_PROJECT}_URL}
		UPDATE_COMMAND ${${EXTERNAL_PROJECT}_UPDATE}
		CONFIGURE_COMMAND "${EXTERNAL_PROJECTS_BASE}/Source/${EXTERNAL_PROJECT}/configure" ${${EXTERNAL_PROJECT}_CONFIGURE}
		BUILD_IN_SOURCE 1
		INSTALL_COMMAND make install)

set(THRIFT_FOUND TRUE)
set(THRIFT_INSTALL ${EXTERNAL_PROJECTS_BASE}/Install/thrift)
set(THRIFT_INCLUDE_DIR ${THRIFT_INSTALL}/include CACHE PATH "thrift include path" FORCE)
set(THRIFT_LIBS ${THRIFT_INSTALL}/lib/libthrift.so CACHE FILEPATH "thrift library" FORCE)
set(THRIFT_STATIC_LIB ${THRIFT_INSTALL}/lib/libthrift.a CACHE FILEPATH "thrift static lib" FORCE)
set(THRIFT_NB_STATIC_LIB ${THRIFT_INSTALL}/lib/libthriftnb.a CACHE FILEPATH "thrift non blocking static lib" FORCE)
set(THRIFT_COMPILER ${THRIFT_INSTALL}/bin/thrift CACHE FILEPATH "thrift compiler" FORCE)

message(STATUS "Thrift include dir: ${THRIFT_INCLUDE_DIR}")
message(STATUS "Thrift library path: ${THRIFT_LIBS}")
message(STATUS "Thrift static library: ${THRIFT_STATIC_LIB}")
message(STATUS "Thrift static non-blocking library: ${THRIFT_NB_STATIC_LIB}")
message(STATUS "Thrift compiler: ${THRIFT_COMPILER}")
