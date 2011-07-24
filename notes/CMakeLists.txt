
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -UQT_NO_CAST_FROM_ASCII -UQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}")

set(akonadinotes_LIB_SRC
  noteutils.cpp
)
kde4_add_library(akonadi-notes ${LIBRARY_TYPE} ${akonadinotes_LIB_SRC})

target_link_libraries(akonadi-notes
                      akonadi-kde
                      kmime)

set_target_properties(akonadi-notes PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS akonadi-notes EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install( FILES
  noteutils.h
  akonadi-notes_export.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/notes COMPONENT Devel
)
