########### next target ###############

add_executable(akonadi_rds)
target_sources(akonadi_rds PRIVATE
    bridgeserver.cpp
    bridgeconnection.cpp
    bridgeserver.h
    bridgeconnection.h
    main.cpp
)

set_target_properties(akonadi_rds PROPERTIES MACOSX_BUNDLE FALSE)

target_link_libraries(akonadi_rds
    akonadi_shared
    KF5AkonadiPrivate
    Qt::Core
    Qt::Network
)

install(TARGETS akonadi_rds
        ${KF_INSTALL_TARGETS_DEFAULT_ARGS}
)

