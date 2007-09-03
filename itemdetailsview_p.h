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

#ifndef AKONADI_ITEMDETAILSVIEW_P_H
#define AKONADI_ITEMDETAILSVIEW_P_H

#include <QtCore/QObject>

#include <libakonadi/datareference.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/monitor.h>

namespace Akonadi {

class ItemDetailsView::Private : public QObject
{
  Q_OBJECT

  public:
    Private( ItemDetailsView *parent )
      : QObject( 0 ),
        mParent( parent ), mMonitor( 0 )
    {
    }

    ~Private()
    {
      delete mMonitor;
    }

    ItemDetailsView *mParent;
    DataReference mUid;
    Monitor *mMonitor;

  private Q_SLOTS:
    void slotItemAdded( const Akonadi::Item &item, const Akonadi::Collection& )
    {
      mParent->itemAdded( item );
    }

    void slotItemChanged( const Akonadi::Item &item, const QStringList& )
    {
      mParent->itemChanged( item );
    }

    void slotItemRemoved( const Akonadi::DataReference& )
    {
      mParent->itemRemoved();
    }

    void initialFetchDone( KJob *job )
    {
      if ( job->error() )
        return;

      ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );

      if ( !fetchJob->items().isEmpty() )
        mParent->itemAdded( fetchJob->items().first() );
    }
};

}

#endif
