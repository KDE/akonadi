
include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${CMAKE_BINARY_DIR}/akonadi
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIR}
    ${AKONADI_INCLUDE_DIR}
    
)

kde_enable_exceptions()

add_library(akonadi_test_searchplugin MODULE testsearchplugin.cpp)

target_link_libraries(akonadi_test_searchplugin
  
  
  ${KDE4_AKONADI_LIBS}
)

install( FILES akonaditestsearchplugin.desktop DESTINATION ${PLUGIN_INSTALL_DIR}/akonadi )
install( TARGETS akonadi_test_searchplugin DESTINATION ${PLUGIN_INSTALL_DIR}/akonadi )
