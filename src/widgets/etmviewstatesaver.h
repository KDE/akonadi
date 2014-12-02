/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

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

#ifndef AKONADI_ETMVIEWSTATESAVER_H
#define AKONADI_ETMVIEWSTATESAVER_H

#include <KConfigViewStateSaver>

#include "collection.h"
#include "item.h"

#include "akonadiwidgets_export.h"

namespace Akonadi {

class AKONADIWIDGETS_EXPORT ETMViewStateSaver : public KConfigViewStateSaver  //krazy:exclude=dpointer
{
    Q_OBJECT
public:
    ETMViewStateSaver(QObject *parent = Q_NULLPTR);

    void selectCollections(const Akonadi::Collection::List &list);
    void selectCollections(const QList<Akonadi::Collection::Id> &list);
    void selectItems(const Akonadi::Item::List &list);
    void selectItems(const QList<Akonadi::Item::Id> &list);

protected:
    /* reimp */
    QModelIndex indexFromConfigString(const QAbstractItemModel *model, const QString &key) const Q_DECL_OVERRIDE;
    /* reimp */ QString indexToConfigString(const QModelIndex &index) const Q_DECL_OVERRIDE;

};

}

#endif
