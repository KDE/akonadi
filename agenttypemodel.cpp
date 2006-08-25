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
    QStringList mMimeTypes;
    org::kde::Akonadi::AgentManager *mManager;

    void init();

    void agentTypeAdded( const QString &agentType );
    void agentTypeRemoved( const QString &agentType );

    void addAgentType( const QString &agentType );
};

void AgentTypeModel::Private::init()
{
  mInfos.clear();

  const QStringList agentTypes = mManager->agentTypes();
  for ( int i = 0; i < agentTypes.count(); ++i )
    addAgentType( agentTypes[ i ] );
}

void AgentTypeModel::Private::addAgentType( const QString &agentType )
{
  if ( !mMimeTypes.isEmpty() ) {
    const QStringList mimeTypes = mManager->agentMimeTypes( agentType );

    /**
     * Check whether the agent type provides one of the mimetypes
     * defined by the filter.
     */
    bool valid = false;
    for ( int i = 0; i < mimeTypes.count(); ++i )
      valid = valid || mMimeTypes.contains( mimeTypes[ i ] );

    if ( !valid )
      return;
  }

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
  d->mManager = new org::kde::Akonadi::AgentManager( "org.kde.Akonadi.Control", "/AgentManager", QDBusConnection::sessionBus(), this );

  d->init();

  connect( d->mManager, SIGNAL( agentTypeAdded( const QString& ) ),
           this, SLOT( agentTypeAdded( const QString& ) ) );
  connect( d->mManager, SIGNAL( agentTypeRemoved( const QString& ) ),
           this, SLOT( agentTypeRemoved( const QString& ) ) );
}

AgentTypeModel::~AgentTypeModel()
{
  delete d;
}

void AgentTypeModel::setFilter( const QStringList &mimeTypes )
{
  d->mMimeTypes = mimeTypes;
  d->init();

  emit layoutChanged();
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
    case Qt::UserRole:
      return info.identifier;
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
