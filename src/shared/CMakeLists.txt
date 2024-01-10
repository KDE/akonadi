# base install include dir
set(AkonadiCore_INSTALL_INCLUDEDIR "${KDE_INSTALL_INCLUDEDIR}/KPim6/AkonadiCore")

add_library(akonadi_shared STATIC)
target_sources(akonadi_shared PRIVATE
    akapplication.cpp
    akdebug.cpp
    akremotelog.cpp
    akapplication.h
    akdebug.h
    akremotelog.h
)
set_property(TARGET akonadi_shared PROPERTY POSITION_INDEPENDENT_CODE ON)
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_shared PROPERTIES UNITY_BUILD ON)
endif()

target_include_directories(akonadi_shared INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_link_libraries(akonadi_shared
PUBLIC
    KPim6::AkonadiPrivate
    Qt::Core
    KF6::Crash
    KF6::I18n
)

ecm_generate_headers(shared_CC_HEADERS
    HEADER_NAMES
    VectorHelper
    REQUIRED_HEADERS shared_HEADERS
    PREFIX Akonadi
)

# shared is not generally a public library, so install only the useful
# public stuff to core
install(FILES
    ${shared_HEADERS}
    DESTINATION ${AkonadiCore_INSTALL_INCLUDEDIR}/akonadi
    COMPONENT Devel
)
install(FILES
    ${shared_CC_HEADERS}
    DESTINATION ${AkonadiCore_INSTALL_INCLUDEDIR}/Akonadi
    COMPONENT Devel
)
