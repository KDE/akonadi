/*
    Copyright (c) 2007-2008 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_ITEMMONITOR_P_H
#define AKONADI_ITEMMONITOR_P_H

#include <QtCore/QObject>

#include <akonadi/itemfetchjob.h>
#include <akonadi/monitor.h>

namespace Akonadi {

class ItemMonitor::Private : public QObject
{
  Q_OBJECT

  public:
    Private( ItemMonitor *parent )
      : QObject( 0 ),
        mParent( parent ), mMonitor( new Monitor() )
    {
      connect( mMonitor, SIGNAL( itemChanged( const Akonadi::Item&, const QStringList& ) ),
               SLOT( slotItemChanged( const Akonadi::Item&, const QStringList& ) ) );
      connect( mMonitor, SIGNAL( itemRemoved( const Akonadi::Item& ) ),
               SLOT( slotItemRemoved( const Akonadi::Item& ) ) );
    }

    ~Private()
    {
      delete mMonitor;
    }

    ItemMonitor *mParent;
    Item mItem;
    Monitor *mMonitor;

  private Q_SLOTS:
    void slotItemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
    {
      mParent->itemChanged( item );
    }

    void slotItemRemoved( const Akonadi::Item& )
    {
      mParent->itemRemoved();
    }

    void initialFetchDone( KJob *job )
    {
      if ( job->error() )
        return;

      ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );

      if ( !fetchJob->items().isEmpty() )
        mParent->itemChanged( fetchJob->items().first() );
    }
};

}

#endif
