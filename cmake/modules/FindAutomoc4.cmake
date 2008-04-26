
# check if we are inside KDESupport and automoc is enabled
if("${KDESupport_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
   # when building this project as part of kdesupport
   set(AUTOMOC4_CONFIG_FILE "${KDESupport_SOURCE_DIR}/automoc/Automoc4Config.cmake")
else("${KDESupport_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
   # when building this project outside kdesupport
   find_file(AUTOMOC4_CONFIG_FILE NAMES Automoc4Config.cmake 
             PATH_SUFFIXES automoc4
             PATHS ${CMAKE_SYSTEM_LIBRARY_PATH} ${CMAKE_INSTALL_PREFIX}/lib 
             NO_DEFAULT_PATH )
endif("${KDESupport_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")


if(AUTOMOC4_CONFIG_FILE)
   include(${AUTOMOC4_CONFIG_FILE})
endif(AUTOMOC4_CONFIG_FILE)
