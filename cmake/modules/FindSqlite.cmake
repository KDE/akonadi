# - Try to find Sqlite
# Once done this will define
#
#  SQLITE_FOUND - system has Sqlite
#  SQLITE_INCLUDE_DIR - the Sqlite include directory
#  SQLITE_LIBRARIES - Link these to use Sqlite
#  SQLITE_DEFINITIONS - Compiler switches required for using Sqlite
#  SQLITE_MIN_VERSION - The minimum SQLite version
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Copyright (c) 2008, Gilles Caulier, <caulier.gilles@gmail.com>
# Copyright (c) 2010, Christophe Giboudeaux, <cgiboudeaux@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(NOT SQLITE_MIN_VERSION)
  set(SQLITE_MIN_VERSION "3.6.16")
endif(NOT SQLITE_MIN_VERSION)

if ( SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES )
   # in cache already
   SET(Sqlite_FIND_QUIETLY TRUE)
endif ( SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES )

find_path(SQLITE_INCLUDE_DIR
          NAMES sqlite3.h
         )

find_library(SQLITE_LIBRARIES
             NAMES sqlite3
            )

if(SQLITE_INCLUDE_DIR)
  file(READ ${SQLITE_INCLUDE_DIR}/sqlite3.h SQLITE3_H_CONTENT)
  string(REGEX MATCH "SQLITE_VERSION[ ]*\"[0-9.]*\"\n" SQLITE_VERSION_MATCH "${SQLITE3_H_CONTENT}")

  if(SQLITE_VERSION_MATCH)
    string(REGEX REPLACE ".*SQLITE_VERSION[ ]*\"(.*)\"\n" "\\1" SQLITE_VERSION ${SQLITE_VERSION_MATCH})

    if(SQLITE_VERSION VERSION_LESS "${SQLITE_MIN_VERSION}")
        message(STATUS "Sqlite ${SQLITE_VERSION} was found, but at least version ${SQLITE_MIN_VERSION} is required")
        set(SQLITE_VERSION_OK FALSE)
    else(SQLITE_VERSION VERSION_LESS "${SQLITE_MIN_VERSION}")
        set(SQLITE_VERSION_OK TRUE)
    endif(SQLITE_VERSION VERSION_LESS "${SQLITE_MIN_VERSION}")

  endif(SQLITE_VERSION_MATCH)

endif(SQLITE_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args( Sqlite DEFAULT_MSG
                                   SQLITE_INCLUDE_DIR
                                   SQLITE_LIBRARIES
                                   SQLITE_VERSION_OK )

# show the SQLITE_INCLUDE_DIR and SQLITE_LIBRARIES variables only in the advanced view
mark_as_advanced( SQLITE_INCLUDE_DIR SQLITE_LIBRARIES )

