project (libakonadixml)

find_package(KDE4 REQUIRED)
find_package (KdepimLibs REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)
include(CheckIncludeFiles)

include_directories(
    ${AKONADI_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${LIBXML2_INCLUDE_DIR}
    ${KDE4_INCLUDES} 
    ${KDEPIMLIBS_INCLUDE_DIR}
)


set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set( libakonadixml_SRCS
  akonadi-xml.cpp
)

kde4_add_library(libakonadixml SHARED ${libakonadixml_SRCS})

target_link_libraries(libakonadixml
  ${KDE4_KIO_LIBS}
  ${KDE4_AKONADI_LIBS}
  ${QT_QTXML_LIBRARY}
  ${LIBXML2_LIBRARIES}
)

set_target_properties(libakonadixml PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})
