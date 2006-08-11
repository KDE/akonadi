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

#include <QtCore/QStringList>

#include <klocale.h>

#include "agentinstancemodel.h"
#include "agentmanagerinterface.h"

using namespace PIM;

class AgentInstanceInfo
{
  public:
    QString identifier;
    QString type;
    QString name;
    QString comment;
    QString icon;
    QStringList mimeTypes;
    QStringList capabilities;
};

class AgentInstanceModel::Private
{
  public:
    Private( AgentInstanceModel *parent )
      : mParent( parent )
    {
    }

    AgentInstanceModel *mParent;
    QList<AgentInstanceInfo> mInfos;
    org::kde::Akonadi::AgentManager *mManager;

    void agentInstanceAdded( const QString &agentInstance );
    void agentInstanceRemoved( const QString &agentInstance );

    void addAgentInstance( const QString &agentInstance );
};

void AgentInstanceModel::Private::addAgentInstance( const QString &agentInstance )
{
  AgentInstanceInfo info;
  info.identifier = agentInstance;
  info.type = mManager->agentInstanceType( agentInstance );
  info.name = mManager->agentName( info.type );
  info.comment = mManager->agentComment( info.type );
  info.icon = mManager->agentIcon( info.type );
  info.mimeTypes = mManager->agentMimeTypes( info.type );
  info.capabilities = mManager->agentCapabilities( info.type );

  mInfos.append( info );
}

void AgentInstanceModel::Private::agentInstanceAdded( const QString &agentInstance )
{
  addAgentInstance( agentInstance );

  emit mParent->layoutChanged();
}

void AgentInstanceModel::Private::agentInstanceRemoved( const QString &agentInstance )
{
  for ( int i = 0; i < mInfos.count(); ++i ) {
    if ( mInfos[ i ].identifier == agentInstance ) {
      mInfos.removeAt( i );
      break;
    }
  }

  emit mParent->layoutChanged();
}

AgentInstanceModel::AgentInstanceModel( QObject *parent )
  : QAbstractItemModel( parent ), d( new Private( this ) )
{
  d->mManager = new org::kde::Akonadi::AgentManager( "org.kde.Akonadi.AgentManager", "/", QDBus::sessionBus(), this );

  const QStringList agentInstances = d->mManager->agentInstances();
  for ( int i = 0; i < agentInstances.count(); ++i )
    d->addAgentInstance( agentInstances[ i ] );

  connect( d->mManager, SIGNAL( agentInstanceAdded( const QString& ) ),
           this, SLOT( agentInstanceAdded( const QString& ) ) );
  connect( d->mManager, SIGNAL( agentInstanceRemoved( const QString& ) ),
           this, SLOT( agentInstanceRemoved( const QString& ) ) );
}

AgentInstanceModel::~AgentInstanceModel()
{
  delete d;
}

int AgentInstanceModel::columnCount( const QModelIndex& ) const
{
  return 1;
}

int AgentInstanceModel::rowCount( const QModelIndex& ) const
{
  return d->mInfos.count();
}

QVariant AgentInstanceModel::data( const QModelIndex &index, int role ) const
{
  if ( !index.isValid() )
    return QVariant();

  if ( index.row() < 0 || index.row() >= d->mInfos.count() )
    return QVariant();

  const AgentInstanceInfo info = d->mInfos[ index.row() ];

  switch ( role ) {
    case Qt::DisplayRole:
      return info.name;
      break;
    case Qt::DecorationRole:
      return info.icon;
      break;
    case Qt::UserRole:
      return info.identifier;
      break;
    case Qt::ToolTipRole:
      return QString( "<qt><h4>%1</h4>%2</qt>" ).arg( info.name, info.comment );
      break;
    default:
      return QVariant();
      break;
  }
}

QVariant AgentInstanceModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( orientation == Qt::Vertical )
    return QVariant();

  if ( role != Qt::DisplayRole )
    return QVariant();

  switch ( section ) {
    case 0:
      return i18n( "Name" );
      break;
    default:
      return QVariant();
      break;
  }
}

QModelIndex AgentInstanceModel::index( int row, int column, const QModelIndex& ) const
{
  if ( row < 0 || row >= d->mInfos.count() )
    return QModelIndex();

  if ( column != 0 )
    return QModelIndex();

  return createIndex( row, column, 0 );
}

QModelIndex AgentInstanceModel::parent( const QModelIndex& ) const
{
  return QModelIndex();
}

#include "agentinstancemodel.moc"
