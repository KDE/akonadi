/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AKONADI_ASYNCSELECTIONHANDLER_P_H
#define AKONADI_ASYNCSELECTIONHANDLER_P_H

#include <QtCore/QObject>

#include "akonadicore_export.h"
#include "collection.h"
#include "item.h"

class QAbstractItemModel;
class QModelIndex;

namespace Akonadi {

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
    explicit AsyncSelectionHandler(QAbstractItemModel *model, QObject *parent = 0);

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

    QAbstractItemModel *mModel;
    Collection mCollection;
    Item mItem;
};

}

#endif
