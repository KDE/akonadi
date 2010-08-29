include_directories(
  ${CMAKE_SOURCE_DIR}/
  ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

if (NOT KDEPIM_NO_KCAL)
set( kcalakonadi_LIB_SRC
  incidencemimetypevisitor.cpp
)

kde4_add_library( akonadi-kcal ${LIBRARY_TYPE} ${kcalakonadi_LIB_SRC} )

target_link_libraries( akonadi-kcal kcal akonadi-kde ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} )
set_target_properties( akonadi-kcal PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadi-kcal EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES
  akonadi-kcal_export.h
  incidencemimetypevisitor.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/kcal COMPONENT Devel
)
endif (NOT KDEPIM_NO_KCAL)
