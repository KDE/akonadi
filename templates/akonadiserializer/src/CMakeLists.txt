set(akonadi_serializer_%{APPNAMELC}_SRCS
    akonadi_serializer_%{APPNAMELC}.cpp
)

add_library(akonadi_serializer_%{APPNAMELC} MODULE ${akonadi_serializer_%{APPNAMELC}_SRCS})

target_link_libraries(akonadi_serializer_%{APPNAMELC}
    KF5::AkonadiCore
)

install(TARGETS akonadi_serializer_%{APPNAMELC}
    DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES akonadi_serializer_%{APPNAMELC}.desktop
    DESTINATION ${KDE_INSTALL_DATADIR}/akonadi/plugins/serializer
)
