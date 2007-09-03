/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#include "itemdetailsview.h"
#include "itemdetailsview_p.h"

using namespace Akonadi;

ItemDetailsView::ItemDetailsView()
  : d( new Private( this ) )
{
}

ItemDetailsView::~ItemDetailsView()
{
  delete d;
}

void ItemDetailsView::setUid( const DataReference &id )
{
  if ( id == d->mUid )
    return;

  d->mUid = id;

  // delete previous monitor
  delete d->mMonitor;

  // create new monitor
  d->mMonitor = new Monitor();

  d->connect( d->mMonitor, SIGNAL( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ),
              d, SLOT( slotItemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ) );
  d->connect( d->mMonitor, SIGNAL( itemChanged( const Akonadi::Item&, const QStringList& ) ),
              d, SLOT( slotItemChanged( const Akonadi::Item&, const QStringList& ) ) );
  d->connect( d->mMonitor, SIGNAL( itemRemoved( const Akonadi::DataReference& ) ),
              d, SLOT( slotItemRemoved( const Akonadi::DataReference& ) ) );

  const QStringList parts = fetchPartIdentifiers();
  for ( int i = 0; i < parts.count(); ++i )
    d->mMonitor->addFetchPart( parts[ i ] );

  d->mMonitor->monitorItem( d->mUid );

  // start initial fetch of the new item
  ItemFetchJob* job = new ItemFetchJob( d->mUid );

  for ( int i = 0; i < parts.count(); ++i )
    job->addFetchPart(  parts[ i ] );
  d->connect( job, SIGNAL( result( KJob* ) ), d, SLOT( initialFetchDone( KJob* ) ) );
}

DataReference ItemDetailsView::uid() const
{
  return d->mUid;
}

void ItemDetailsView::itemAdded( const Item& )
{
}

void ItemDetailsView::itemChanged( const Item& )
{
}

void ItemDetailsView::itemRemoved()
{
}

QStringList ItemDetailsView::fetchPartIdentifiers() const
{
  return QStringList( Item::PartBody );
}

#include "itemdetailsview_p.moc"
