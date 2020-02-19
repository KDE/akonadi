set(selftest_SRCS
    main.cpp
)

add_executable(akonadiselftest ${selftest_SRCS})

target_link_libraries(akonadiselftest
PRIVATE
    KF5::AkonadiWidgets
    KF5::AkonadiPrivate
    KF5::I18n
    Qt5::Sql
    Qt5::Widgets
)

install(TARGETS
    akonadiselftest
    ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)
