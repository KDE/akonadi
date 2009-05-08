project (akonadixml)

macro_optional_find_package(LibXml2)
macro_log_feature(LIBXML2_FOUND "LibXML2" "Libraries used to develop XML applications" "http://xmlsoft.org" FALSE "" "Needed for building the knut Akonadi resource.")

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
  xmlwritejob.cpp
)

kde4_add_library(akonadi-xml SHARED ${akonadixml_SRCS})

target_link_libraries(akonadi-xml
  ${KDE4_KIO_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${QT_QTXML_LIBRARY}
  ${LIBXML2_LIBRARIES}
)

set_target_properties(akonadi-xml PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})
install (TARGETS akonadi-xml ${INSTALL_TARGETS_DEFAULT_ARGS})
install( FILES
   akonadi-xml_export.h
   xmldocument.h
   xmlreader.h
   xmlwriter.h
   xmlwritejob.h
   DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/xml/ COMPONENT Devel
)
install( FILES akonadi-xml.xsd DESTINATION ${DATA_INSTALL_DIR}/akonadi/ )

kde4_add_executable(akonadi2xml akonadi2xml.cpp)
target_link_libraries(akonadi2xml
   akonadi-xml
   ${KDE4_KDEUI_LIBS}
   ${KDEPIMLIBS_AKONADI_LIBS}
)
install( TARGETS akonadi2xml ${INSTALL_TARGETS_DEFAULT_ARGS} )
