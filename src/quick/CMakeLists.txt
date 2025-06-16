# SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
# SPDX-License-Identifier: BSD-2-Clause

ecm_add_qml_module(akonadi_quick_plugin
    URI "org.kde.akonadi"
    GENERATE_PLUGIN_SOURCE
    DEPENDENCIES
        org.kde.kirigami
        org.kde.kirigamiaddons.delegates
        org.kde.kirigamiaddons.formcard
        org.kde.kirigamiaddons.components
        org.kde.kitemmodels
)

qt_extract_metatypes(akonadi_quick_plugin)

target_sources(akonadi_quick_plugin PRIVATE
    agentconfiguration.cpp
    agentconfiguration.h
    colorproxymodel.cpp
    colorproxymodel.h
    collectioncomboboxmodel.cpp
    collectioncomboboxmodel.h
    collectionpickermodel.cpp
    collectionpickermodel.h
    collectioneditorcontroller.cpp
    collectioneditorcontroller.h
    mimetypes.cpp
    mimetypes.h
    tagmanager.h
    tagmanager.cpp
    types.cpp
    types.h
)

ecm_target_qml_sources(akonadi_quick_plugin SOURCES
    qml/AgentConfigurationForm.qml
    qml/CollectionComboBox.qml
    qml/CollectionChooserPage.qml
    qml/FormCollectionComboBox.qml
    qml/TagManagerPage.qml
)

ecm_qt_declare_logging_category(akonadi_quick_plugin
    HEADER akonadi_quick_debug.h
    IDENTIFIER AKONADI_QUICK_LOG
    CATEGORY_NAME org.kde.pim.akonadiquick
    DESCRIPTION "Akonadi QtQuick Plugin"
    EXPORT AKONADI
)

target_link_libraries(akonadi_quick_plugin PRIVATE
    KF6::WindowSystem
    KF6::I18n
    KPim6AkonadiCore
    KPim6AkonadiWidgets # Needed for AgentConfigurationDialog
)

ecm_finalize_qml_module(akonadi_quick_plugin
    DESTINATION ${KDE_INSTALL_QMLDIR}
    BUILD_SHARED_LIBS OFF
)
