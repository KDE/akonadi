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

#include "profilemodel.h"
#include "profilemanager.h"

using namespace Akonadi;

class ProfileInfo
{
  public:
    QString identifier;
    QStringList agents;
};

class ProfileModel::Private
{
  public:
    Private( ProfileModel *parent )
      : mParent( parent )
    {
    }

    ProfileModel *mParent;
    QList<ProfileInfo> mInfos;
    ProfileManager mManager;

    void profileAdded( const QString &profile );
    void profileRemoved( const QString &profile );
    void profileAgentAdded( const QString &profile, const QString &agent );
    void profileAgentRemoved( const QString &profile, const QString &agent );

    void addProfile( const QString &profile );
};

void ProfileModel::Private::addProfile( const QString &profile )
{
  ProfileInfo info;
  info.identifier = profile;
  info.agents = mManager.profileAgents( profile );

  mInfos.append( info );
}

void ProfileModel::Private::profileAdded( const QString &profile )
{
  addProfile( profile );

  emit mParent->layoutChanged();
}

void ProfileModel::Private::profileRemoved( const QString &profile )
{
  for ( int i = 0; i < mInfos.count(); ++i ) {
    if ( mInfos[ i ].identifier == profile ) {
      mInfos.removeAt( i );
      break;
    }
  }

  emit mParent->layoutChanged();
}

void ProfileModel::Private::profileAgentAdded( const QString &profile, const QString &agent )
{
  for ( int i = 0; i < mInfos.count(); ++i ) {
    if ( mInfos[ i ].identifier == profile ) {
      mInfos[ i ].agents.append( agent );

      const QModelIndex index = mParent->index( i, 0 );
      emit mParent->dataChanged( index, index );
      break;
    }
  }
}

void ProfileModel::Private::profileAgentRemoved( const QString &profile, const QString &agent )
{
  for ( int i = 0; i < mInfos.count(); ++i ) {
    if ( mInfos[ i ].identifier == profile ) {
      mInfos[ i ].agents.removeAll( agent );

      const QModelIndex index = mParent->index( i, 0 );
      emit mParent->dataChanged( index, index );
      break;
    }
  }
}

ProfileModel::ProfileModel( QObject *parent )
  : QAbstractItemModel( parent ), d( new Private( this ) )
{
  const QStringList profiles = d->mManager.profiles();
  for ( int i = 0; i < profiles.count(); ++i )
    d->addProfile( profiles[ i ] );

  connect( &d->mManager, SIGNAL( profileAdded( const QString& ) ),
           this, SLOT( profileAdded( const QString& ) ) );
  connect( &d->mManager, SIGNAL( profileRemoved( const QString& ) ),
           this, SLOT( profileRemoved( const QString& ) ) );
  connect( &d->mManager, SIGNAL( profileAgentAdded( const QString&, const QString& ) ),
           this, SLOT( profileAgentAdded( const QString&, const QString& ) ) );
  connect( &d->mManager, SIGNAL( profileAgentRemoved( const QString&, const QString& ) ),
           this, SLOT( profileAgentRemoved( const QString&, const QString& ) ) );
}

ProfileModel::~ProfileModel()
{
  delete d;
}

int ProfileModel::columnCount( const QModelIndex& ) const
{
  return 1;
}

int ProfileModel::rowCount( const QModelIndex& ) const
{
  return d->mInfos.count();
}

QVariant ProfileModel::data( const QModelIndex &index, int role ) const
{
  if ( !index.isValid() )
    return QVariant();

  if ( index.row() < 0 || index.row() >= d->mInfos.count() )
    return QVariant();

  const ProfileInfo info = d->mInfos[ index.row() ];

  switch ( role ) {
    case Qt::DisplayRole:
      return QString::fromLatin1( "%1 (%2)" ).arg( info.identifier, info.agents.join( QLatin1String( ", " ) ) );
      break;
    case Qt::UserRole:
      return info.identifier;
      break;
    default:
      return QVariant();
      break;
  }
}

QModelIndex ProfileModel::index( int row, int column, const QModelIndex& ) const
{
  if ( row < 0 || row >= d->mInfos.count() )
    return QModelIndex();

  if ( column != 0 )
    return QModelIndex();

  return createIndex( row, column, 0 );
}

QModelIndex ProfileModel::parent( const QModelIndex& ) const
{
  return QModelIndex();
}

#include "profilemodel.moc"
