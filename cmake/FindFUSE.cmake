# - Find FUSE (Filesystem in User Space)
# This module defines
# FUSE_INCLUDE_DIR, where to find FUSE headers
# FUSE_LIBRARY, FUSE library
# FUSE_FOUND, true or false

find_path(FUSE_INCLUDE_DIR fuse/fuse.h)

find_library(FUSE_LIBRARY fuse)

if(FUSE_INCLUDE_DIR AND FUSE_LIBRARY)
	set(FUSE_FOUND TRUE)
	if(NOT FUSE_FIND_QUIETLY)
		message(STATUS "FUSE library: ${FUSE_LIBRARY}")
	endif()
else()
	set(FUSE_FOUND FALSE)
	message(FATAL_ERROR "FUSE was not found")
endif()

mark_as_advanced(
	FUSE_INCLUDE_DIR
	FUSE_LIBRARY)
