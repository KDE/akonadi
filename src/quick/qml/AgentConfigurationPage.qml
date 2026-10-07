// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.ki18n
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.akonadi

FormCard.FormCardPage {
    id: root

    required property AgentConfigurationSession configuration

    KI18nContext {
        id: _tr
        translationDomain: "libakonadi6"
    }

    footer: Controls.ToolBar {
        contentItem: Controls.DialogButtonBox {
            standardButtons: Controls.DialogButtonBox.Cancel
            onRejected: root.closeDialog()

            Controls.Button {
                text: _tr.i18nc("@action:button", "Save")
                enabled: root.configuration.canSave
                onClicked: root.configuration.save()
            }
        }
    }
}
