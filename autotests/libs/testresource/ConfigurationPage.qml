// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.akonadi as Akonadi

Akonadi.AgentConfigurationPage {
    id: root

    readonly property KnutConfigQuick settings: configuration.controller as KnutConfigQuick

    title: _tr.i18nc("@title:window", "Knut Configuration")

    KI18nContext {
        id: _tr
        translationDomain: "akonadi_knut_resource"
    }

    FormCard.FormCard {
        autoSeparators: true

        Layout.topMargin: Kirigami.Units.gridUnit

        FormCard.FormFileDelegate {
            label: _tr.i18nc("@label", "Data Files:")
            fileMode: FileDialog.SaveFile
            nameFilters: [_tr.i18nc("@item:inlistbox", "Akonadi Knut Data File (*.xml)")]
            onAccepted: root.settings.setDataFileUrl(selectedFile)
        }

        FormCard.FormCheckDelegate {
            text: _tr.i18nc("@option:check", "File watching enabled")
            checked: root.settings.fileWatchingEnabled
            onToggled: root.settings.fileWatchingEnabled = checked
        }
    }
}
