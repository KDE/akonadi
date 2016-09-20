include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

########### next target ###############

set(akonadictl_SRCS
    akonadistarter.cpp
    main.cpp
)

ecm_qt_declare_logging_category(akonadictl_SRCS HEADER akonadictl_debug.h IDENTIFIER AKONADICTL_LOG CATEGORY_NAME org.kde.pim.akonadictl)

qt5_add_dbus_interfaces(akonadictl_SRCS
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.ControlManager.xml
    ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.Janitor.xml
)

add_executable(akonadictl ${akonadictl_SRCS})
set_target_properties(akonadictl PROPERTIES OUTPUT_NAME akonadictl)
target_link_libraries(akonadictl
    akonadi_shared
    KF5AkonadiPrivate
    Qt5::Core
    Qt5::DBus
)

install(TARGETS akonadictl
        ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}}
)
