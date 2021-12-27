add_executable(akonadiselftest main.cpp)

target_link_libraries(akonadiselftest
PRIVATE
    KF5::AkonadiWidgets
    KF5::AkonadiPrivate
    KF5::I18n
    Qt${QT_MAJOR_VERSION}::Sql
    Qt${QT_MAJOR_VERSION}::Widgets
)

install(TARGETS
    akonadiselftest
    ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)
