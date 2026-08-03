/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net
    SPDX-FileContributor: Stephen Kelly <stephen@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KConfigViewStateSaver>

// AkonadiCore
#include "akonadi/collection.h"
#include "akonadi/item.h"

#include "akonadiwidgets_export.h"

namespace Akonadi
{
/*!
 * \class Akonadi::ETMViewStateSaver
 * \inheaderfile Akonadi/ETMViewStateSaver
 * \inmodule AkonadiWidgets
 */
class AKONADIWIDGETS_EXPORT ETMViewStateSaver : public KConfigViewStateSaver
{
    Q_OBJECT
public:
    /*!
     * Creates a new ETM view state saver.
     * \a parent The parent object.
     */
    explicit ETMViewStateSaver(QObject *parent = nullptr);

    /*!
     * How checked/selected collections are keyed in the config.
     */
    enum KeyFormat {
        IdKeys, ///< By numeric collection id ("c<id>"). Default. Not stable if a resource re-lists collections with new ids.
        RemotePathKeys, ///< By resource identifier + chain of remoteIds ("r<resource>/<rid>/..."). Stable across id renumbering.
    };

    /*!
     * Sets how checked/selected collections are written to the config.
     *
     * Only affects saving; reading always understands both formats, so a config
     * written with \c IdKeys migrates automatically to \c RemotePathKeys on the
     * next save. \a format The key format to write.
     */
    void setKeyFormat(KeyFormat format);
    /*!
     * Returns the key format used when saving.
     */
    [[nodiscard]] KeyFormat keyFormat() const;

    /*!
     * Reimplemented to keep stable (\c RemotePathKeys) entries of the existing configuration
     * whose collections are currently missing from the model.
     *
     * Restoring is asynchronous, so an application that saves on shutdown would otherwise
     * overwrite the stored selection with an empty one when it quits before the model is
     * populated. Kept entries are limited to path keys; a missing id key is still dropped,
     * because a stale id can later resolve to a different collection. \a configGroup The group
     * to save to.
     */
    void saveState(KConfigGroup &configGroup);

    /*!
     * Selects the given collections in the view.
     * \a list The list of collections to select.
     */
    void selectCollections(const Akonadi::Collection::List &list);
    /*!
     * Selects the collections with the given IDs in the view.
     * \a list The list of collection IDs to select.
     */
    void selectCollections(const QList<Akonadi::Collection::Id> &list);
    /*!
     * Selects the given items in the view.
     * \a list The list of items to select.
     */
    void selectItems(const Akonadi::Item::List &list);
    /*!
     * Selects the items with the given IDs in the view.
     * \a list The list of item IDs to select.
     */
    void selectItems(const QList<Akonadi::Item::Id> &list);

    /*!
     * Sets the current item in the view.
     * \a item The item to set as current.
     */
    void setCurrentItem(const Akonadi::Item &item);
    /*!
     * Sets the current collection in the view.
     * \a collection The collection to set as current.
     */
    void setCurrentCollection(const Akonadi::Collection &collection);

protected:
    /* reimp */
    QModelIndex indexFromConfigString(const QAbstractItemModel *model, const QString &key) const override;
    QString indexToConfigString(const QModelIndex &index) const override;

private:
    /// Builds the stable "r<resource>/<rid>/..." key for a collection index, or an empty string
    /// if it has no usable remote path (item, or a collection with an empty remoteId anywhere in the chain).
    [[nodiscard]] QString stableKeyForIndex(const QModelIndex &index) const;
    /// Depth-first search for the collection index whose stableKeyForIndex() equals \a key.
    [[nodiscard]] QModelIndex indexForStableKey(const QAbstractItemModel *model, const QString &key, const QModelIndex &parent) const;

    KeyFormat mKeyFormat = IdKeys;
};

}
