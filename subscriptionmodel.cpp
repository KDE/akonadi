/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "subscriptionmodel.h"
#include "collectionlistjob.h"

#include <kdebug.h>

using namespace Akonadi;

class SubscriptionModel::Private
{
  public:
    Private( SubscriptionModel* parent ) : q( parent ) {}
    SubscriptionModel* q;
    QHash<int, bool> subscriptions;
    QSet<int> changes;

    Collection::List changedSubscriptions( bool subscribed )
    {
      Collection::List list;
      foreach ( int id, changes ) {
        if ( subscriptions.value( id ) == subscribed )
          list << Collection( id );
      }
      return list;
    }

    void listResult( KJob* job )
    {
      if ( job->error() ) {
        // TODO
        kWarning() << job->errorString();
        return;
      }
      Collection::List cols = static_cast<CollectionListJob*>( job )->collections();
      foreach( const Collection col, cols )
        if ( col.type() != Collection::Structural )
          subscriptions[ col.id() ] = true;
      q->reset();
      emit q->loaded();
    }

    bool isSubscribable( int id )
    {
      Collection col = q->collectionForId( id );
      if ( col.type() == Collection::VirtualParent || col.type() == Collection::Structural )
        return false;
      if ( col.contentTypes().isEmpty() )
        return false;
      return true;
    }
};

SubscriptionModel::SubscriptionModel(QObject * parent) :
    CollectionModel( parent ),
    d( new Private( this ) )
{
  includeUnsubscribed();
  CollectionListJob* job = new CollectionListJob( Collection::root(), CollectionListJob::Recursive, this );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(listResult(KJob*)) );
}

SubscriptionModel::~ SubscriptionModel()
{
  delete d;
}

QVariant SubscriptionModel::data(const QModelIndex & index, int role) const
{
  switch ( role ) {
    case Qt::CheckStateRole:
    {
      int col = index.data( CollectionIdRole ).toInt();
      if ( !d->isSubscribable( col ) )
        return QVariant();
      if ( d->subscriptions.value( col ) )
        return Qt::Checked;
      return Qt::Unchecked;
    }
    case SubscriptionChangedRole:
    {
      int col = index.data( CollectionIdRole ).toInt();
      if ( d->changes.contains( col ) )
        return true;
      return false;
    }
  }
  return CollectionModel::data( index, role );
}

Qt::ItemFlags SubscriptionModel::flags(const QModelIndex & index) const
{
  Qt::ItemFlags flags = CollectionModel::flags( index );
  if ( d->isSubscribable( index.data( CollectionIdRole ).toInt() ) )
    return flags | Qt::ItemIsUserCheckable;
  return flags;
}

bool SubscriptionModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if ( role == Qt::CheckStateRole ) {
    int col = index.data( CollectionIdRole ).toInt();
    if ( d->subscriptions.contains( col ) && d->subscriptions.value( col ) == (value == Qt::Checked) )
      return true; // no change
    d->subscriptions[ col ] = value == Qt::Checked;
    if ( d->changes.contains( col ) )
      d->changes.remove( col );
    else
      d->changes.insert( col );
    emit dataChanged( index, index );
    return true;
  }
  return CollectionModel::setData( index, value, role );
}

Collection::List SubscriptionModel::subscribed() const
{
  return d->changedSubscriptions( true );
}

Collection::List SubscriptionModel::unsubscribed() const
{
  return d->changedSubscriptions( false );
}

#include "subscriptionmodel.moc"
