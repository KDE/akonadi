include_directories(
         ${CMAKE_SOURCE_DIR}/
         ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( kabcakonadi_LIB_SRC
  contactparts.cpp
)

add_library( akonadi-kabc ${LIBRARY_TYPE} ${kabcakonadi_LIB_SRC} )

generate_export_header(akonadi-kabc
  BASE_NAME akonadi_kabc
  EXPORT_FILE_NAME akonadi-kabc_export.h
)

target_link_libraries( akonadi-kabc akonadi-kde ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS} )
set_target_properties( akonadi-kabc PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadi-kabc EXPORT kdepimlibsLibraryTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES
  ${CMAKE_CURRENT_BINARY_DIR}/akonadi-kabc_export.h
  contactparts.h
  DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/akonadi/kabc COMPONENT Devel
)
