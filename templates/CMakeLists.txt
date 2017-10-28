set(apptemplate_DIRS
    akonadiresource
    akonadiserializer
)

kde_package_app_templates(TEMPLATES ${apptemplate_DIRS} INSTALL_DIR ${KDE_INSTALL_KTEMPLATESDIR})
