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
  localfolders.cpp
  localfoldersbuildjob.cpp
  messagemodel.cpp
  messageparts.cpp
  messagethreadingattribute.cpp
  messagethreaderproxymodel.cpp
  resourcesynchronizationjob.cpp # copied from playground/pim/akonaditest/resourcetester
)

kde4_add_kcfg_files( kmimeakonadi_LIB_SRC localfolderssettings.kcfgc )
install( FILES localfolders.kcfg DESTINATION ${KCFG_INSTALL_DIR} )

kde4_add_library( akonadi-kmime SHARED ${kmimeakonadi_LIB_SRC} )

target_link_libraries( akonadi-kmime akonadi-kde kmime ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} )
set_target_properties( akonadi-kmime PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadi-kmime EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES
  addressattribute.h
  localfolders.h
  akonadi-kmime_export.h
  messagemodel.h
  messageparts.h
  messagethreadingattribute.h
  messagethreaderproxymodel.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/kmime COMPONENT Devel
)
