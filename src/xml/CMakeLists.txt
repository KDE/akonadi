
find_package(LibXml2)
set_package_properties(LibXml2 PROPERTIES
    DESCRIPTION "Required for XML schema validation in akonadixml"
    URL "http://xmlsoft.org"
    TYPE OPTIONAL
)

if (LIBXML2_FOUND)
    add_definitions(-DHAVE_LIBXML2)
endif ()

if (BUILD_TESTING)
   add_subdirectory(autotests)
endif()
set(akonadixml_SRCS
    xmldocument.cpp xmldocument.h
    xmlreader.cpp xmlreader.h
    xmlwritejob.cpp xmlwritejob.h
    xmlwriter.cpp xmlwriter.h
)

# base install include dir
set(AkonadiXml_INSTALL_INCLUDEDIR "${KDE_INSTALL_INCLUDEDIR}/KPim6/AkonadiXml")

ecm_generate_headers(AkonadiXml_CC_HEADERS
    HEADER_NAMES
    XmlDocument
    XmlReader
    XmlWriteJob
    XmlWriter
    REQUIRED_HEADERS AkonadiXml_HEADERS
    PREFIX Akonadi
)

add_executable(akonadi2xml akonadi2xml.cpp)
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi2xml PROPERTIES UNITY_BUILD ON)
endif()

set_target_properties(akonadi2xml PROPERTIES MACOSX_BUNDLE FALSE)

target_link_libraries(akonadi2xml
    KPim6::AkonadiXml
    KF6::I18n
    Qt::Widgets # for QApplication
)

add_library(KPim6AkonadiXml ${akonadixml_SRCS})

generate_export_header(KPim6AkonadiXml BASE_NAME akonadi-xml)

add_library(KPim6::AkonadiXml ALIAS KPim6AkonadiXml)

target_include_directories(KPim6AkonadiXml INTERFACE "$<INSTALL_INTERFACE:${AkonadiXml_INSTALL_INCLUDEDIR}>")
target_include_directories(KPim6AkonadiXml PUBLIC "$<BUILD_INTERFACE:${Akonadi_SOURCE_DIR}/src/core;${Akonadi_BINARY_DIR}/src/core>")

target_link_libraries(KPim6AkonadiXml
PUBLIC
    KPim6::AkonadiCore
    Qt::Xml
PRIVATE
    KF6::I18n
)
if (LIBXML2_FOUND)
    target_link_libraries(KPim6AkonadiXml PRIVATE LibXml2::LibXml2)
endif()

set_target_properties(KPim6AkonadiXml PROPERTIES
    VERSION ${AKONADI_VERSION}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiXml
)

ecm_generate_pri_file(BASE_NAME AkonadiXml
    LIB_NAME KPim6AkonadiXml
    DEPS "AkonadiCore QtXml" FILENAME_VAR PRI_FILENAME
)

install(TARGETS
    KPim6AkonadiXml
    EXPORT KPim6AkonadiTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(TARGETS
    akonadi2xml ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadi-xml_export.h
    ${AkonadiXml_HEADERS}
    DESTINATION ${AkonadiXml_INSTALL_INCLUDEDIR}/akonadi
    COMPONENT Devel
)
install(FILES
    ${AkonadiXml_CC_HEADERS}
    DESTINATION ${AkonadiXml_INSTALL_INCLUDEDIR}/Akonadi
    COMPONENT Devel
)

install(FILES
    ${PRI_FILENAME}
    DESTINATION ${ECM_MKSPECS_INSTALL_DIR}
)

install(FILES
    akonadi-xml.xsd
    DESTINATION ${KDE_INSTALL_DATADIR_KF}/akonadi/
)
