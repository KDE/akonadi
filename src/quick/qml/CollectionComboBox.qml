// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.akonadi as Akonadi
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.delegates as Delegates

/**
 * Special combobox control that allows to choose a collection.
 * The collection displayed can be filtered using the \p mimeTypeFilter
 * and \p accessRightsFilter properties.
 */
QQC2.ComboBox {
    id: root

    /**
     * This property holds the id of the default collection, that is the
     * collection that will be selected by default.
     * @property int defaultCollectionId
     */
    property alias defaultCollectionId: collectionComboBoxModel.defaultCollectionId

    /**
     * This property holds the mime types of the collection that should be
     * displayed.
     *
     * @property list<string> mimeTypeFilter
     * @code{.qml}
     * import org.kde.akonadi as Akonadi
     *
     * Akonadi.CollectionComboBoxModel {
     *     mimeTypeFilter: [Akonadi.MimeTypes.address, Akonadi.MimeTypes.contactGroup]
     * }
     * @endcode
     */
    property alias mimeTypeFilter: collectionComboBoxModel.mimeTypeFilter

    /**
     * This property holds the access right of the collection that should be
     * displayed.
     *
     * @property Akonadi::Collection::Rights rights
     * @code{.qml}
     * import org.kde.akonadi as Akonadi
     *
     * Akonadi.CollectionComboBoxModel {
     *     accessRightsFilter: Akonadi.Collection.CanCreateItem
     * }
     * @endcode
     */
    property alias accessRightsFilter: collectionComboBoxModel.accessRightsFilter

    signal userSelectedCollection(var collection)

    currentIndex: 0
    onActivated: index => { if (index > -1) {
        const selectedModelIndex = collectionComboBoxModel.index(currentIndex, 0);
        const selectedCollection = collectionComboBoxModel.data(selectedModelIndex, Akonadi.EntityTreeModel.CollectionRole);
        userSelectedCollection(selectedCollection);
    }
    }

    textRole: "display"
    valueRole: "collectionId"

    indicator: Rectangle {
        id: indicatorDot

        // Make sure to check the currentValue property directly or risk listening to something that won't necessarily emit a changed() signal'
        readonly property var selectedModelIndex: root.currentValue > -1 ? root.model.index(root.currentIndex, 0) : null
        readonly property var selectedCollectionColor: root.currentValue > -1 ? root.model.data(selectedModelIndex, Akonadi.EntityTreeModel.CollectionColorRole) : null

        implicitHeight: root.implicitHeight * 0.4
        implicitWidth: implicitHeight

        x: root.mirrored ? root.leftPadding : root.width - (root.leftPadding * 3) - Kirigami.Units.iconSizes.smallMedium
        y: root.topPadding + (root.availableHeight - height) / 2

        radius: width
        color: selectedCollectionColor
    }

    model: Akonadi.CollectionComboBoxModel {
        id: collectionComboBoxModel
        onCurrentIndexChanged: root.currentIndex = currentIndex
    }

    delegate: Delegates.RoundedItemDelegate {
        id: delegate

        required property int index
        required property string displayName
        required property string decoration
        required property color collectionColor

        text: displayName
        icon.name: decoration

        Rectangle {
            anchors.margins: Kirigami.Units.smallSpacing
            width: height
            radius: height
            color: delegate.collectionColor
        }
    }

    popup.z: 1000
}
