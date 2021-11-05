add_executable(asapcat)
target_sources(asapcat PRIVATE
    main.cpp
    session.cpp
    session.h
    )
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
