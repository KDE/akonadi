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
    KF5::AkonadiCore
    KF5::KDELibs4Support
    Qt5::Sql
)

install(TARGETS
    akonadiselftest
    ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)
