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

#include "agenttypemodel.h"
#include "agentmanagerinterface.h"

using namespace PIM;

class AgentTypeInfo
{
  public:
    QString identifier;
    QString name;
    QString comment;
    QString icon;
    QStringList mimeTypes;
    QStringList capabilities;
};

class AgentTypeModel::Private
{
  public:
    Private( AgentTypeModel *parent )
      : mParent( parent )
    {
    }

    AgentTypeModel *mParent;
    QList<AgentTypeInfo> mInfos;
    org::kde::Akonadi::AgentManager *mManager;

    void agentTypeAdded( const QString &agentType );
    void agentTypeRemoved( const QString &agentType );

    void addAgentType( const QString &agentType );
};

void AgentTypeModel::Private::addAgentType( const QString &agentType )
{
  AgentTypeInfo info;
  info.identifier = agentType;
  info.name = mManager->agentName( agentType );
  info.comment = mManager->agentComment( agentType );
  info.icon = mManager->agentIcon( agentType );
  info.mimeTypes = mManager->agentMimeTypes( agentType );
  info.capabilities = mManager->agentCapabilities( agentType );

  mInfos.append( info );
}

void AgentTypeModel::Private::agentTypeAdded( const QString &agentType )
{
  addAgentType( agentType );

  emit mParent->layoutChanged();
}

void AgentTypeModel::Private::agentTypeRemoved( const QString &agentType )
{
  for ( int i = 0; i < mInfos.count(); ++i ) {
    if ( mInfos[ i ].identifier == agentType ) {
      mInfos.removeAt( i );
      break;
    }
  }

  emit mParent->layoutChanged();
}

AgentTypeModel::AgentTypeModel( QObject *parent )
  : QAbstractItemModel( parent ), d( new Private( this ) )
{
  d->mManager = new org::kde::Akonadi::AgentManager( "org.kde.Akonadi.AgentManager", "/", QDBus::sessionBus(), this );

  const QStringList agentTypes = d->mManager->agentTypes();
  for ( int i = 0; i < agentTypes.count(); ++i )
    d->addAgentType( agentTypes[ i ] );

  connect( d->mManager, SIGNAL( agentTypeAdded( const QString& ) ),
           this, SLOT( agentTypeAdded( const QString& ) ) );
  connect( d->mManager, SIGNAL( agentTypeRemoved( const QString& ) ),
           this, SLOT( agentTypeRemoved( const QString& ) ) );
}

AgentTypeModel::~AgentTypeModel()
{
  delete d;
}

int AgentTypeModel::columnCount( const QModelIndex& ) const
{
  return 1;
}

int AgentTypeModel::rowCount( const QModelIndex& ) const
{
  return d->mInfos.count();
}

QVariant AgentTypeModel::data( const QModelIndex &index, int role ) const
{
  if ( !index.isValid() )
    return QVariant();

  if ( index.row() < 0 || index.row() >= d->mInfos.count() )
    return QVariant();

  const AgentTypeInfo info = d->mInfos[ index.row() ];

  switch ( role ) {
    case Qt::DisplayRole:
      return info.name;
      break;
    case Qt::DecorationRole:
      return info.icon;
      break;
    case CommentRole:
      return info.comment;
      break;
    case MimeTypesRole:
      return info.mimeTypes;
      break;
    case CapabilitiesRole:
      return info.capabilities;
      break;
    default:
      return QVariant();
      break;
  }
}

QModelIndex AgentTypeModel::index( int row, int column, const QModelIndex& ) const
{
  if ( row < 0 || row >= d->mInfos.count() )
    return QModelIndex();

  if ( column != 0 )
    return QModelIndex();

  return createIndex( row, column, 0 );
}

QModelIndex AgentTypeModel::parent( const QModelIndex& ) const
{
  return QModelIndex();
}

#include "agenttypemodel.moc"
