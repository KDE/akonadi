/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#ifndef PROFILEMODEL_H
#define PROFILEMODEL_H

#include "libakonadi_export.h"
#include <QtCore/QAbstractItemModel>

namespace Akonadi {

class AKONADI_EXPORT ProfileModel : public QAbstractItemModel
{
  Q_OBJECT

  public:
    explicit ProfileModel( QObject *parent );
    virtual ~ProfileModel();

    virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const;
    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;

    virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;

    virtual QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;
    virtual QModelIndex parent( const QModelIndex &index ) const;

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void profileAdded( const QString& ) )
    Q_PRIVATE_SLOT( d, void profileRemoved( const QString& ) )
    Q_PRIVATE_SLOT( d, void profileAgentAdded( const QString&, const QString& ) )
    Q_PRIVATE_SLOT( d, void profileAgentRemoved( const QString&, const QString& ) )
};

}

#endif
