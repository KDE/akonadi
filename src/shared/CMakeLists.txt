set(akonadi_shared_srcs
    akapplication.cpp
    akdebug.cpp
    akremotelog.cpp
)

add_library(akonadi_shared STATIC ${akonadi_shared_srcs})
target_include_directories(akonadi_shared INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/Optional;${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/MPark.Variant>")
target_include_directories(akonadi_shared INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_link_libraries(akonadi_shared
    Qt5::Core
    KF5AkonadiPrivate
    KF5::Crash
)

ecm_generate_headers(shared_HEADERS
    HEADER_NAMES
    AkOptional
    VectorHelper
    REQUIRED_HEADERS shared_HEADERS
)

# shared is not generally a public library, so install only the useful
# public stuff to core
install(FILES
    ${shared_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/AkonadiCore COMPONENT Devel
)
