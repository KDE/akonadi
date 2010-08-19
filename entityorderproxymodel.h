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

#ifndef AKONADI_ENTITYORDERPROXYMODEL_H
#define AKONADI_ENTITYORDERPROXYMODEL_H

#include <QtGui/QSortFilterProxyModel>

#include "akonadi_export.h"

class KConfigGroup;

namespace Akonadi
{
class EntityOrderProxyModelPrivate;

class AKONADI_EXPORT EntityOrderProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  EntityOrderProxyModel(QObject* parent = 0);
  
  virtual ~EntityOrderProxyModel();

  void setOrderConfig( KConfigGroup &configGroup );

  void saveOrder();

  void clearOrder( const QModelIndex &index );
  void clearTreeOrder();

  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const;

  virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);

protected:
  EntityOrderProxyModelPrivate * const d_ptr;

  virtual QString parentConfigString( const QModelIndex &index ) const;
  virtual QString configString( const QModelIndex &index ) const;

private:
  Q_DECLARE_PRIVATE( EntityOrderProxyModel )
};

}

#endif
