include_directories(
         ${CMAKE_SOURCE_DIR}/
         ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( kabcakonadi_LIB_SRC
  contactparts.cpp
)

kde4_add_library( akonadi-kabc SHARED ${kabcakonadi_LIB_SRC} )

target_link_libraries( akonadi-kabc akonadi-kde ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS} )
set_target_properties( akonadi-kabc PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadi-kabc ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES
  akonadi-kabc_export.h
  contactparts.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/kabc COMPONENT Devel
)
