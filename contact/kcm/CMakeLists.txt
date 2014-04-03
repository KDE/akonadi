
########### next target ###############

set(kcm_akonadicontact_actions_SRCS
    kcmakonadicontactactions.cpp)

qt4_wrap_ui(kcm_akonadicontact_actions_SRCS akonadicontactactions.ui)

configure_file( ../actions/contactactionssettings.kcfg.cmake ${CMAKE_CURRENT_BINARY_DIR}/contactactionssettings.kcfg @ONLY)
kconfig_add_kcfg_files(kcm_akonadicontact_actions_SRCS ../actions/contactactionssettings.kcfgc)

kde4_add_plugin(kcm_akonadicontact_actions ${kcm_akonadicontact_actions_SRCS})

target_link_libraries(kcm_akonadicontact_actions ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS})

install(TARGETS kcm_akonadicontact_actions DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install(FILES akonadicontact_actions.desktop DESTINATION ${SERVICES_INSTALL_DIR})
