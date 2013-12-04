# - Find Thrift (a cross platform RPC lib/tool)
# This module defines
# THRIFT_VERSION, version string of ant if found
# THRIFT_INCLUDE_DIR, where to find THRIFT headers
# THRIFT_LIBS, THRIFT libraries
# THRIFT_FOUND, true or false

# prefer the thrift version supplied in THRIFT_HOME
message(STATUS "THRIFT_HOME: $ENV{THRIFT_HOME}")
find_path(THRIFT_INCLUDE_DIR thrift/Thrift.h HINTS
  $ENV{THRIFT_HOME}/include/
  /usr/local/include/
  /opt/local/include/
)

set(THRIFT_LIB_PATHS
  $ENV{THRIFT_HOME}/lib
  /usr/local/lib
  /opt/local/lib)

find_path(THRIFT_STATIC_LIB_PATH libthrift.a PATHS ${THRIFT_LIB_PATHS})

# prefer the thrift version supplied in THRIFT_HOME
find_library(THRIFT_LIB NAMES thrift HINTS ${THRIFT_LIB_PATHS})

find_program(THRIFT_COMPILER thrift
  $ENV{THRIFT_HOME}/bin
  /usr/local/bin
  /usr/bin
  NO_DEFAULT_PATH
)

if (THRIFT_LIB)
  set(THRIFT_FOUND TRUE)
  set(THRIFT_LIBS ${THRIFT_LIB})
  set(THRIFT_STATIC_LIB ${THRIFT_STATIC_LIB_PATH}/libthrift.a)
  set(THRIFT_NB_STATIC_LIB ${THRIFT_STATIC_LIB_PATH}/libthriftnb.a)
  exec_program(${THRIFT_COMPILER}
    ARGS -version OUTPUT_VARIABLE THRIFT_VERSION RETURN_VALUE THRIFT_RETURN)
else ()
  set(THRIFT_FOUND FALSE)
endif ()

if (THRIFT_FOUND)
  if (NOT THRIFT_FIND_QUIETLY)
    message(STATUS "Thrift version: ${THRIFT_VERSION}")
  endif ()
endif ()

mark_as_advanced(
  THRIFT_LIB
  THRIFT_COMPILER
  THRIFT_INCLUDE_DIR
)
