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

#ifndef AGENTINSTANCEMODEL_H
#define AGENTINSTANCEMODEL_H

#include <QtCore/QAbstractItemModel>
#include <kdepim_export.h>

namespace Akonadi {

class AKONADI_EXPORT AgentInstanceModel : public QAbstractItemModel
{
  Q_OBJECT

  public:
    enum Role
    {
      StatusRole = Qt::UserRole + 1,
      StatusMessageRole,
      ProgressRole,
      ProgressMessageRole,
      OnlineRole
    };

    AgentInstanceModel( QObject *parent );
    virtual ~AgentInstanceModel();

    virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const;
    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;

    virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    virtual QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;
    virtual QModelIndex parent( const QModelIndex &index ) const;

    virtual Qt::ItemFlags flags( const QModelIndex &index ) const;
    virtual bool setData( const QModelIndex &index, const QVariant &value, int role );

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void agentInstanceAdded( const QString& ) )
    Q_PRIVATE_SLOT( d, void agentInstanceRemoved( const QString& ) )
    Q_PRIVATE_SLOT( d, void agentInstanceStatusChanged( const QString&, AgentManager::Status, const QString& ) )
    Q_PRIVATE_SLOT( d, void agentInstanceProgressChanged( const QString&, uint, const QString& ) )
    Q_PRIVATE_SLOT( d, void agentInstanceNameChanged( const QString&, const QString& ) )
};

}

#endif
