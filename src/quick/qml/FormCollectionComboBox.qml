// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.akonadi as Akonadi
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.kirigamiaddons.delegates as Delegates

/**
 * Special combobox control that allows to choose a collection.
 * The collection displayed can be filtered using the \p mimeTypeFilter
 * and \p accessRightsFilter properties.
 */
FormCard.FormComboBoxDelegate {
    id: comboBox

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

    readonly property var collection: if (currentIndex >= 0) {
        const selectedModelIndex = collectionComboBoxModel.index(currentIndex, 0);
        return collectionComboBoxModel.data(selectedModelIndex, Akonadi.EntityTreeModel.CollectionRole);
    } else {
        null
    }

    signal userSelectedCollection(var collection)

    currentIndex: 0
    onActivated: index => { if (currentIndex > -1) {
        const selectedModelIndex = collectionComboBoxModel.index(currentIndex, 0);
        const selectedCollection = collectionComboBoxModel.data(selectedModelIndex, Akonadi.EntityTreeModel.CollectionRole);
        userSelectedCollection(selectedCollection);
        }
    }

    textRole: "display"
    valueRole: "collectionId"

    spacing: Kirigami.Units.smallSpacing

    indicator: Rectangle {
        id: indicatorDot

        // Make sure to check the currentValue property directly or risk listening to something that won't necessarily emit a changed() signal'
        readonly property var selectedModelIndex: comboBox.currentValue > -1 ? comboBox.model.index(comboBox.currentIndex, 0) : null
        readonly property var selectedCollectionColor: comboBox.currentValue > -1 ? comboBox.model.data(selectedModelIndex, Akonadi.EntityTreeModel.CollectionColorRole) : null

        implicitHeight: comboBox.implicitHeight * 0.2
        implicitWidth: implicitHeight

        x: comboBox.mirrored ? comboBox.leftPadding : comboBox.leftPadding + comboBox.availableWidth + comboBox.spacing
        y: comboBox.topPadding + (comboBox.availableHeight - height) / 2

        radius: width * 0.5
        color: selectedCollectionColor
    }

    model: Akonadi.CollectionComboBoxModel {
        id: collectionComboBoxModel
        onCurrentIndexChanged: comboBox.currentIndex = currentIndex
    }

    comboBoxDelegate: Delegates.RoundedItemDelegate {
        text: model.display
        icon.source: decoration
        Rectangle {
            anchors.margins: Kirigami.Units.smallSpacing
            width: height
            radius: width * 0.5
            color: model.collectionColor
        }
    }
}
