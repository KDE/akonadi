set(asapcat_srcs
    main.cpp
    session.cpp
)

add_executable(asapcat ${asapcat_srcs})
set_target_properties(asapcat PROPERTIES MACOSX_BUNDLE FALSE)

target_link_libraries(asapcat
    akonadi_shared
    KF5AkonadiPrivate
    Qt::Core
    Qt::Network
)

install(TARGETS asapcat
        ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)
