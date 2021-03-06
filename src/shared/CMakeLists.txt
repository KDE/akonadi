add_library(akonadi_shared STATIC)
target_sources(akonadi_shared PRIVATE
    akapplication.cpp
    akdebug.cpp
    akremotelog.cpp
)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_shared PROPERTIES UNITY_BUILD ON)
endif()

target_include_directories(akonadi_shared INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_link_libraries(akonadi_shared
PUBLIC
    KF5AkonadiPrivate
    Qt::Core
    KF5::Crash
)

ecm_generate_headers(shared_HEADERS
    HEADER_NAMES
    VectorHelper
    REQUIRED_HEADERS shared_HEADERS
)

# shared is not generally a public library, so install only the useful
# public stuff to core
install(FILES
    ${shared_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/AkonadiCore COMPONENT Devel
)
