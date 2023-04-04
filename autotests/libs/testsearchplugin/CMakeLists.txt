kde_enable_exceptions()

add_library(akonadi_test_searchplugin MODULE testsearchplugin.cpp testsearchplugin.h)

target_link_libraries(akonadi_test_searchplugin KPim6::AkonadiCore)

install( TARGETS akonadi_test_searchplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim6/akonadi )
