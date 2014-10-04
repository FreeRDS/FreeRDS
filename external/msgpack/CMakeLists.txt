
cmake_minimum_required(VERSION 2.8.6)

project(msgpack)

set(VERSION 0.5.9)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)

list(APPEND msgpack_SOURCES
	src/unpack.c
	src/objectc.c
	src/version.c
	src/vrefbuffer.c
	src/zone.c)

list(APPEND msgpack_HEADERS
	src/msgpack/pack_define.h
	src/msgpack/pack_template.h
	src/msgpack/unpack_define.h
	src/msgpack/unpack_template.h
	src/msgpack/sysdep.h
	src/msgpack/sbuffer.h
	src/msgpack/version.h
	src/msgpack/vrefbuffer.h
	src/msgpack/zbuffer.h
	src/msgpack/fbuffer.h
	src/msgpack/pack.h
	src/msgpack/unpack.h
	src/msgpack/object.h
	src/msgpack/zone.h)

configure_file(
	src/msgpack/version.h.in
	${CMAKE_CURRENT_BINARY_DIR}/src/msgpack/version.h)

include_directories(
	./
	src/
	src/msgpack/
	${CMAKE_CURRENT_BINARY_DIR}/src/
	${CMAKE_CURRENT_BINARY_DIR}/src/msgpack/)

add_library(msgpack STATIC
	${msgpack_SOURCES}
	${msgpack_HEADERS})

set_target_properties(msgpack PROPERTIES OUTPUT_NAME "msgpack")
