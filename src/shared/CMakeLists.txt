set(akonadi_shared_srcs
    akapplication.cpp
    akcrash.cpp
    akdbus.cpp
    akdebug.cpp
)

add_library(akonadi_shared STATIC ${akonadi_shared_srcs})

target_link_libraries(akonadi_shared
    Qt5::Core
    Qt5::DBus
    KF5AkonadiPrivate
    ${Backtrace_LIBRARIES}
)
