
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII)

set(akonadinotes_LIB_SRC
  noteutils.cpp
)
add_library(akonadi-notes ${LIBRARY_TYPE} ${akonadinotes_LIB_SRC})
if(CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 2.8.12)
  set_property(TARGET akonadi-notes PROPERTY DEFINE_SYMBOL akonadi_notes)
endif()
generate_export_header(akonadi-notes
  BASE_NAME akonadi_notes
  EXPORT_FILE_NAME akonadi-notes_export.h
)

include_directories(
  ${CMAKE_BINARY_DIR}/kmime
)


target_link_libraries(akonadi-notes
                      akonadi-kde
                      kmime ${QT_QTXML_LIBRARY})

set_target_properties(akonadi-notes PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS akonadi-notes EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install( FILES
  noteutils.h
  ${CMAKE_CURRENT_BINARY_DIR}/akonadi-notes_export.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/notes COMPONENT Devel
)

add_subdirectory(tests)
