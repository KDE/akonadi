add_executable(akonadi_agent_server)
# Agent server
target_sources(akonadi_agent_server PRIVATE
    agentpluginloader.cpp
    agentserver.cpp
    agentthread.cpp
    main.cpp
)

ecm_qt_declare_logging_category(akonadi_agent_server HEADER akonadiagentserver_debug.h IDENTIFIER AKONADIAGENTSERVER_LOG CATEGORY_NAME org.kde.pim.akonadiagentserver
        DESCRIPTION "akonadi (Akonadi Agent Server)"
        OLD_CATEGORY_NAMES log_akonadiagentserver
        EXPORT AKONADI
    )


if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_agent_server PROPERTIES UNITY_BUILD ON)
endif()

set_target_properties(akonadi_agent_server PROPERTIES MACOSX_BUNDLE FALSE)

target_link_libraries(akonadi_agent_server
    akonadi_shared
    KF5AkonadiPrivate
    Qt::Core
    Qt::DBus
)

# Agent plugin launcher
add_executable(akonadi_agent_launcher)
target_sources(akonadi_agent_launcher PRIVATE
    agentpluginloader.cpp
    agentlauncher.cpp
    akonadiagentserver_debug.cpp
)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_agent_launcher PROPERTIES UNITY_BUILD ON)
endif()

set_target_properties(akonadi_agent_launcher PROPERTIES MACOSX_BUNDLE FALSE)

target_link_libraries(akonadi_agent_launcher
    akonadi_shared
    KF5AkonadiPrivate
    Qt::Core
    Qt::Widgets
)

# Install both helper apps.
install(TARGETS akonadi_agent_launcher
          DESTINATION ${BIN_INSTALL_DIR})

install(TARGETS akonadi_agent_server
        ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
