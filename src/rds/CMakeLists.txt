########### next target ###############

set(akonadi_rds_srcs
    bridgeserver.cpp
    bridgeconnection.cpp
    main.cpp
)

add_executable(akonadi_rds ${akonadi_rds_srcs})
set_target_properties(akonadi_rds PROPERTIES MACOSX_BUNDLE FALSE)

target_link_libraries(akonadi_rds
    akonadi_shared
    KF5AkonadiPrivate
    Qt5::Core
    Qt5::Network
)

install(TARGETS akonadi_rds
        ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)

