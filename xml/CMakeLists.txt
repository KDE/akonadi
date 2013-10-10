if ( LIBXML2_FOUND )
  add_definitions( -DHAVE_LIBXML2 )
endif ()

add_subdirectory( tests )

include_directories(
    ${KDE4_INCLUDES}
    ${LIBXML2_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set( akonadixml_SRCS
  xmldocument.cpp
  xmlreader.cpp
  xmlwriter.cpp
  xmlwritejob.cpp
)

set( akonadixml_HDRS
  ${CMAKE_CURRENT_BINARY_DIR}/akonadi-xml_export.h
  xmldocument.h
  xmlreader.h
  xmlwritejob.h
  xmlwriter.h
)

kde4_add_library(akonadi-xml ${LIBRARY_TYPE} ${akonadixml_SRCS})
generate_export_header(akonadi-xml BASE_NAME akonadi_xml EXPORT_FILE_NAME akonadi-xml_export.h)

target_link_libraries(akonadi-xml
  ${KDE4_KIO_LIBS}
  ${QT_QTXML_LIBRARY}
  ${LIBXML2_LIBRARIES}
  akonadi-kde
)

set_target_properties(akonadi-xml PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS akonadi-xml EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${akonadixml_HDRS} DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/xml COMPONENT Devel)
install(FILES akonadi-xml.xsd DESTINATION ${DATA_INSTALL_DIR}/akonadi/)

kde4_add_executable(akonadi2xml akonadi2xml.cpp)
target_link_libraries(akonadi2xml
   akonadi-xml
   ${KDE4_KDEUI_LIBS}
   akonadi-kde
)

install(TARGETS akonadi2xml ${INSTALL_TARGETS_DEFAULT_ARGS})
