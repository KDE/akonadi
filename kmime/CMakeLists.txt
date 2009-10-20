include_directories(
        ${CMAKE_SOURCE_DIR}/
        ${QT_QTDBUS_INCLUDE_DIR}
        ${Boost_INCLUDE_DIR}
)

add_subdirectory( tests )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( kmimeakonadi_LIB_SRC
  addressattribute.cpp
  specialcollectionattribute.cpp
  specialcollections.cpp
  specialcollectionshelperjobs.cpp
  specialcollectionsrequestjob.cpp
  messagefolderattribute.cpp
  messagemodel.cpp
  messageparts.cpp
  messagethreadingattribute.cpp
  messagethreaderproxymodel.cpp
)

# TODO figure out how to only add this if we're building unit tests.
set( kmimeakonadi_LIB_SRC
  ${kmimeakonadi_LIB_SRC}
  specialcollectionstesting.cpp
)

kde4_add_kcfg_files( kmimeakonadi_LIB_SRC specialcollectionssettings.kcfgc )
install( FILES specialcollections.kcfg DESTINATION ${KCFG_INSTALL_DIR} )

kde4_add_library( akonadi-kmime SHARED ${kmimeakonadi_LIB_SRC} )

target_link_libraries( akonadi-kmime akonadi-kde kmime ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} )
set_target_properties( akonadi-kmime PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadi-kmime EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES
  addressattribute.h
  specialcollections.h
  specialcollectionsrequestjob.h
  akonadi-kmime_export.h
  messagefolderattribute.h
  messagemodel.h
  messageparts.h
  messagethreadingattribute.h
  messagethreaderproxymodel.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/kmime COMPONENT Devel
)
