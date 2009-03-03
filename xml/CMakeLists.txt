project (akonadixml)    

find_package(KDE4 REQUIRED)
find_package (KdepimLibs REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)
include(CheckIncludeFiles)

add_subdirectory( tests )

include_directories(
    ${AKONADI_INCLUDE_DIR}
    ${KDE4_INCLUDES} 
    ${KDEPIMLIBS_INCLUDE_DIR}
)


set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set( akonadixml_SRCS
  akonadi-xml.cpp
)

kde4_add_library(akonadixml SHARED ${akonadixml_SRCS})

target_link_libraries(akonadixml
  ${KDE4_KIO_LIBS}
  ${KDE4_AKONADI_LIBS}
  ${QT_QTXML_LIBRARY}
)

set_target_properties(akonadixml PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})
install (TARGETS akonadixml ${INSTALL_TARGETS_DEFAULT_ARGS})

