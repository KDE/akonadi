/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ASYNCSELECTIONHANDLER_P_H
#define AKONADI_ASYNCSELECTIONHANDLER_P_H

#include <QObject>

#include "akonadicore_export.h"
#include "collection.h"
#include "item.h"

class QAbstractItemModel;
class QModelIndex;

namespace Akonadi
{

/**
 * @internal
 *
 * A helper class to set a current index on a widget with
 * delayed model loading.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADICORE_EXPORT AsyncSelectionHandler : public QObject
{
    Q_OBJECT

public:
    /**
     */
    explicit AsyncSelectionHandler(QAbstractItemModel *model, QObject *parent = nullptr);

    ~AsyncSelectionHandler();

    void waitForCollection(const Collection &collection);
    void waitForItem(const Item &item);

Q_SIGNALS:
    void collectionAvailable(const QModelIndex &index);
    void itemAvailable(const QModelIndex &index);

private Q_SLOTS:
    void rowsInserted(const QModelIndex &parent, int start, int end);

private:
    bool scanSubTree(const QModelIndex &index, bool searchForItem);

    QAbstractItemModel *mModel = nullptr;
    Collection mCollection;
    Item mItem;
};

}

#endif
