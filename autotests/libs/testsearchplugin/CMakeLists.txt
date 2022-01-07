kde_enable_exceptions()

add_library(akonadi_test_searchplugin MODULE testsearchplugin.cpp testsearchplugin.h)

target_link_libraries(akonadi_test_searchplugin KF5::AkonadiCore)

install( TARGETS akonadi_test_searchplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/akonadi )
