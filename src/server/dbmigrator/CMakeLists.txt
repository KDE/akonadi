ecm_qt_declare_logging_category(migrator_SRCS HEADER akonadidbmigrator_debug.h IDENTIFIER AKONADIDBMIGRATOR_LOG CATEGORY_NAME org.kde.pim.akonadiserver.dbmigrator
        DESCRIPTION "akonadi (Akonadi Server Database Migrator)"
        EXPORT AKONADI
)


qt_add_dbus_interface(migrator_SRCS ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.ControlManager.xml ControlManager)

add_executable(akonadi-db-migrator)
target_sources(akonadi-db-migrator
    PRIVATE
    main.cpp
    dbmigrator.cpp
    ${migrator_SRCS}
)

target_link_libraries(akonadi-db-migrator
    libakonadiserver
    KF6::CoreAddons
    KF6::I18n
)

install(TARGETS akonadi-db-migrator
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)
