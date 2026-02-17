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
};

}
