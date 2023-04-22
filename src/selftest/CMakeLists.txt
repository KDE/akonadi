add_executable(akonadiselftest main.cpp)

target_link_libraries(akonadiselftest
PRIVATE
    KPim${KF_MAJOR_VERSION}::AkonadiWidgets
    KPim${KF_MAJOR_VERSION}::AkonadiPrivate
    KF${KF_MAJOR_VERSION}::I18n
    Qt::Sql
    Qt::Widgets
)

install(TARGETS
    akonadiselftest
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)
