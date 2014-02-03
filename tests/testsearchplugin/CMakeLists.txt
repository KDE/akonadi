
include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${CMAKE_BINARY_DIR}/akonadi
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIR}
    ${AKONADI_INCLUDE_DIR}
    ${KDE4_INCLUDES}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

kde4_add_plugin(akonadi_test_searchplugin testsearchplugin.cpp)

target_link_libraries(akonadi_test_searchplugin
  akonadi-kde
  ${KDE4_KDEUI_LIBS}
  ${KDE4_AKONADI_LIBS}
)

install( FILES akonaditestsearchplugin.desktop DESTINATION ${PLUGIN_INSTALL_DIR}/akonadi )
install( TARGETS akonadi_test_searchplugin DESTINATION ${PLUGIN_INSTALL_DIR}/akonadi )
