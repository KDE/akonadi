set(selftest_SRCS
    selftestdialog.cpp
    main.cpp
)

set(selftest_UI
    selftestdialog.ui
)

qt5_wrap_ui(selftest_SRCS ${selftest_UI})

add_executable(akonadiselftest ${selftest_SRCS})

target_link_libraries(akonadiselftest
    KF5::AkonadiCore
    KF5::KDE4Support
    Qt5::Sql
)

install(TARGETS
    akonadiselftest
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)
