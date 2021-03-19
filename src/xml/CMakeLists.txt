
find_package(LibXml2)
set_package_properties(LibXml2 PROPERTIES
    DESCRIPTION "Required for XML schema validation in akonadixml"
    URL "http://xmlsoft.org"
    TYPE OPTIONAL
)

if (LIBXML2_FOUND)
    add_definitions(-DHAVE_LIBXML2)
    # TODO: Use LibXml2::LibXml2 when we'll require CMake >= 3.12
    include_directories(${LIBXML2_INCLUDE_DIR})
endif ()

if (BUILD_TESTING)
   add_subdirectory(autotests)
endif()
set(akonadixml_SRCS
    xmldocument.cpp
    xmlreader.cpp
    xmlwritejob.cpp
    xmlwriter.cpp
)

ecm_generate_headers(AkonadiXml_HEADERS
    HEADER_NAMES
    XmlDocument
    XmlReader
    XmlWriteJob
    XmlWriter
    REQUIRED_HEADERS AkonadiXml_HEADERS
)

add_executable(akonadi2xml akonadi2xml.cpp)
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi2xml PROPERTIES UNITY_BUILD ON)
endif()

set_target_properties(akonadi2xml PROPERTIES MACOSX_BUNDLE FALSE)

target_link_libraries(akonadi2xml
    KF5::AkonadiXml
    KF5::I18n
    Qt5::Widgets # for QApplication
)

add_library(KF5AkonadiXml ${akonadixml_SRCS})

generate_export_header(KF5AkonadiXml BASE_NAME akonadi-xml)

add_library(KF5::AkonadiXml ALIAS KF5AkonadiXml)

target_include_directories(KF5AkonadiXml INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF5}/AkonadiXml>")
target_include_directories(KF5AkonadiXml PUBLIC "$<BUILD_INTERFACE:${Akonadi_SOURCE_DIR}/src/core;${Akonadi_BINARY_DIR}/src/core>")

target_link_libraries(KF5AkonadiXml
PUBLIC
    KF5::AkonadiCore
    Qt5::Xml
PRIVATE
    KF5::I18n
    ${LIBXML2_LIBRARIES}
)

set_target_properties(KF5AkonadiXml PROPERTIES
    VERSION ${AKONADI_VERSION_STRING}
    SOVERSION ${AKONADI_SOVERSION}
    EXPORT_NAME AkonadiXml
)

ecm_generate_pri_file(BASE_NAME AkonadiXml
    LIB_NAME KF5AkonadiXml
    DEPS "AkonadiCore QtXml" FILENAME_VAR PRI_FILENAME
)

install(TARGETS
    KF5AkonadiXml
    EXPORT KF5AkonadiTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(TARGETS
    akonadi2xml ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akonadi-xml_export.h
    ${AkonadiXml_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/AkonadiXml COMPONENT Devel
)

install(FILES
    ${PRI_FILENAME}
    DESTINATION ${ECM_MKSPECS_INSTALL_DIR}
)

install(FILES
    akonadi-xml.xsd
    DESTINATION ${KDE_INSTALL_DATADIR_KF5}/akonadi/
)
