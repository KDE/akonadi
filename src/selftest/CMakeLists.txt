add_executable(akonadiselftest main.cpp)

target_link_libraries(akonadiselftest
PRIVATE
    KF5::AkonadiWidgets
    KF5::AkonadiPrivate
    KF${KF_MAJOR_VERSION}::I18n
    Qt::Sql
    Qt::Widgets
)

install(TARGETS
    akonadiselftest
    ${KF_INSTALL_TARGETS_DEFAULT_ARGS}
)
