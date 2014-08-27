set(asapcat_srcs
    main.cpp
    session.cpp
)

add_executable(asapcat ${asapcat_srcs})

target_link_libraries(asapcat
    akonadi_shared
    KF5AkonadiPrivate
    Qt5::Core
    Qt5::Network
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
)

install(TARGETS asapcat
        ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)
