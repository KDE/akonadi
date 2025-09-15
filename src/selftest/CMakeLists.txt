add_executable(akonadiselftest main.cpp)

target_link_libraries(
    akonadiselftest
    PRIVATE
        KPim6::AkonadiWidgets
        KPim6::AkonadiPrivate
        KF6::I18n
        Qt::Sql
        Qt::Widgets
)

install(
    TARGETS
        akonadiselftest
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)
