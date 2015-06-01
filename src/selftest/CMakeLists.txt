set(selftest_SRCS
    selftestdialog.cpp
    main.cpp
)

set(selftest_UI
    selftestdialog.ui
)

ki18n_wrap_ui(selftest_SRCS ${selftest_UI})

add_executable(akonadiselftest ${selftest_SRCS})

target_link_libraries(akonadiselftest
PRIVATE
    KF5::AkonadiCore
    KF5::AkonadiPrivate
    KF5::DBusAddons
    KF5::I18n
    KF5::KDELibs4Support # for KFileDialog
    Qt5::Sql
    Qt5::Widgets
)

install(TARGETS
    akonadiselftest
    ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)
