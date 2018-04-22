set(akonadi_shared_srcs
    akapplication.cpp
    akdebug.cpp
    akremotelog.cpp
)

add_library(akonadi_shared STATIC ${akonadi_shared_srcs})

target_link_libraries(akonadi_shared
    Qt5::Core
    KF5AkonadiPrivate
    KF5::Crash
)
