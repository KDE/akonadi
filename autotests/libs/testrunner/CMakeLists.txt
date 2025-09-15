kde_enable_exceptions()

set(akonaditest_SRCS
    main.cpp
    setup.cpp
    setup.h
    config.cpp
    shellscript.cpp
    shellscript.h
    testrunner.cpp
    testrunner.h
)

ecm_qt_declare_logging_category(akonaditest_SRCS HEADER akonaditest_debug.h IDENTIFIER AKONADITEST_LOG CATEGORY_NAME org.kde.pim.akonaditest)

add_executable(akonaditest ${akonaditest_SRCS})

target_link_libraries(
    akonaditest
    KPim6::AkonadiPrivate
    KPim6::AkonadiCore
    KF6::I18n
    KF6::ConfigCore
    Qt::Xml
    Qt::DBus
)

install(
    TARGETS
        akonaditest
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

# Set the akonaditest path (needed by AkonadiMacros.cmake when invoked in kdepimlibs)
set(_akonaditest_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "akonaditest path")
