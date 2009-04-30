# Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

set (LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)")

if (WIN32)
  # use relative install prefix to avoid hardcoded install paths in cmake_install.cmake files

  set (EXEC_INSTALL_PREFIX "") # Base directory for executables and libraries

  ## ${CMAKE_INSTALL_PREFIX}/*
  set (BIN_INSTALL_DIR             "bin")                     # The install dir for executables (default ${EXEC_INSTALL_PREFIX}/bin)
  set (INCLUDE_INSTALL_DIR         "include")                 # The subdirectory to the header prefix
  set (LIB_INSTALL_DIR             "lib${LIB_SUFFIX}")        # The subdirectory relative to the install prefix where libraries will be installed (default is ${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX})
  set (SHARE_INSTALL_PREFIX        "share")                   # Base directory for files which go to share/

  ## ${CMAKE_INSTALL_PREFIX}/share/*
  set (CONFIG_INSTALL_DIR          "share/config")            # The config file install dir
  set (DBUS_INTERFACES_INSTALL_DIR "share/dbus-1/interfaces") # The DBus interfaces install dir (default ${SHARE_INSTALL_PREFIX}/dbus-1/interfaces)")
  set (DBUS_SERVICES_INSTALL_DIR   "share/dbus-1/services")   # The DBus services install dir (default ${SHARE_INSTALL_PREFIX}/dbus-1/services)")
  set (XDG_MIME_INSTALL_DIR        "share/mime/packages")     # The install dir for the xdg mimetypes

else (WIN32)
  # this macro implements some very special logic how to deal with the cache
  # by default the various install locations inherit their value from theit "parent" variable
  # so if you set CMAKE_INSTALL_PREFIX, then EXEC_INSTALL_PREFIX, PLUGIN_INSTALL_DIR will
  # calculate their value by appending subdirs to CMAKE_INSTALL_PREFIX
  # this would work completely without using the cache.
  # but if somebody wants e.g. a different EXEC_INSTALL_PREFIX this value has to go into
  # the cache, otherwise it will be forgotten on the next cmake run.
  # Once a variable is in the cache, it doesn't depend on its "parent" variables
  # anymore and you can only change it by editing it directly.
  # this macro helps in this regard, because as long as you don't set one of the
  # variables explicitely to some location, it will always calculate its value from its
  # parents. So modifying CMAKE_INSTALL_PREFIX later on will have the desired effect.
  # But once you decide to set e.g. EXEC_INSTALL_PREFIX to some special location
  # this will go into the cache and it will no longer depend on CMAKE_INSTALL_PREFIX.
  macro(_SET_FANCY _var _value _comment)
    SET (predefinedvalue "${_value}")

    if (NOT DEFINED ${_var})
      SET (${_var} ${predefinedvalue})
    else (NOT DEFINED ${_var})
      SET (${_var} "${${_var}}" CACHE PATH "${_comment}")
    endif (NOT DEFINED ${_var})
  endmacro(_SET_FANCY)


  _set_fancy(EXEC_INSTALL_PREFIX  "${CMAKE_INSTALL_PREFIX}"                 "Base directory for executables and libraries")

  ## ${CMAKE_INSTALL_PREFIX}/*
  _set_fancy(BIN_INSTALL_DIR      "${EXEC_INSTALL_PREFIX}/bin"              "The install dir for executables (default ${EXEC_INSTALL_PREFIX}/bin)")
  _set_fancy(INCLUDE_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}/include"         "The subdirectory to the header prefix")
  _set_fancy(LIB_INSTALL_DIR      "${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX}" "The subdirectory relative to the install prefix where libraries will be installed (default is ${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX})")
  _set_fancy(SHARE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/share"           "Base directory for files which go to share/")


  ## ${CMAKE_INSTALL_PREFIX}/share/*
  _set_fancy(CONFIG_INSTALL_DIR          "${SHARE_INSTALL_PREFIX}/config"            "The config file install dir")
  _set_fancy(DBUS_INTERFACES_INSTALL_DIR "${SHARE_INSTALL_PREFIX}/dbus-1/interfaces" "The DBus interfaces install dir (default ${SHARE_INSTALL_PREFIX}/dbus-1/interfaces)")
  _set_fancy(DBUS_SERVICES_INSTALL_DIR   "${SHARE_INSTALL_PREFIX}/dbus-1/services"   "The DBus services install dir (default ${SHARE_INSTALL_PREFIX}/dbus-1/services)")
  _set_fancy(XDG_MIME_INSTALL_DIR        "${SHARE_INSTALL_PREFIX}/mime/packages"     "The install dir for the xdg mimetypes")

endif (WIN32)

# The INSTALL_TARGETS_DEFAULT_ARGS variable should be used when libraries are installed.
# The arguments are also ok for regular executables, i.e. executables which don't go
# into sbin/ or libexec/, but for installing executables the basic syntax
# INSTALL(TARGETS kate DESTINATION "${BIN_INSTALL_DIR}")
# is enough, so using this variable there doesn't help a lot.
# The variable must not be used for installing plugins.
# Usage is like this:
# install(TARGETS kdecore kdeui ${INSTALL_TARGETS_DEFAULT_ARGS})
#
# This will install libraries correctly under UNIX, OSX and Windows (i.e. dll's go
# into bin/.
# Later on it will be possible to extend this for installing OSX frameworks
# The COMPONENT Devel argument has the effect that static libraries belong to the
# "Devel" install component. If we use this also for all install() commands
# for header files, it will be possible to install
# -everything: make install OR cmake -P cmake_install.cmake
# -only the development files: cmake -DCOMPONENT=Devel -P cmake_install.cmake
# -everything except the development files: cmake -DCOMPONENT=Unspecified -P cmake_install.cmake
# This can then also be used for packaging with cpack.

SET (INSTALL_TARGETS_DEFAULT_ARGS RUNTIME DESTINATION "${BIN_INSTALL_DIR}"
LIBRARY DESTINATION "${LIB_INSTALL_DIR}"
ARCHIVE DESTINATION "${LIB_INSTALL_DIR}" COMPONENT Devel)
