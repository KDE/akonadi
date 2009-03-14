project (akonadixml)    

find_package(KDE4 REQUIRED)
find_package (KdepimLibs REQUIRED)
find_package( LibXml2 )
include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)
include(CheckIncludeFiles)

if ( LIBXML2_FOUND )
  add_definitions( -DHAVE_LIBXML2 )
endif ( LIBXML2_FOUND )

add_subdirectory( tests )

add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

include_directories(
    ${AKONADI_INCLUDE_DIR}
    ${KDE4_INCLUDES} 
    ${KDEPIMLIBS_INCLUDE_DIR}
    ${LIBXML2_INCLUDE_DIR}
)


set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set( akonadixml_SRCS
  xmldocument.cpp
  xmlreader.cpp
  xmlwriter.cpp
)

kde4_add_library(akonadixml SHARED ${akonadixml_SRCS})

target_link_libraries(akonadixml
  ${KDE4_KIO_LIBS}
  ${KDE4_AKONADI_LIBS}
  ${QT_QTXML_LIBRARY}
  ${LIBXML2_LIBRARIES}
)

set_target_properties(akonadixml PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})
install (TARGETS akonadixml ${INSTALL_TARGETS_DEFAULT_ARGS})
install( FILES
   akonadi-xml_export.h
   xmldocument.h
   xmlreader.h
   xmlwriter.h
   DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/ COMPONENT Devel
)
