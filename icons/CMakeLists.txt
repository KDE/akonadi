include(ECMInstallIcons)

set(icons
    16-apps-akonadi.png
    22-apps-akonadi.png
    32-apps-akonadi.png
    48-apps-akonadi.png
    64-apps-akonadi.png
    128-apps-akonadi.png
    256-apps-akonadi.png
    sc-apps-akonadi.svgz
)

ecm_install_icons(ICONS ${icons} DESTINATION ${ICON_INSTALL_DIR})
