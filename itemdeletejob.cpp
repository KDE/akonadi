/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "itemdeletejob.h"
#include "itemstorejob.h"
#include "expungejob.h"
#include "transactionjobs.h"

using namespace Akonadi;

class ItemDeleteJob::Private
{
  public:
    enum State {
      Begin,
      Store,
      Expunge,
      Commit
    };

    Private( ItemDeleteJob *parent )
      : mParent( parent )
    {
    }

    void jobDone( KJob* );

    ItemDeleteJob *mParent;
    DataReference ref;
    State state;
};

void ItemDeleteJob::Private::jobDone( KJob * job )
{
  if ( !job->error() ) // error is already handled by KCompositeJob
    mParent->emitResult();
}

ItemDeleteJob::ItemDeleteJob( const DataReference & ref, QObject * parent )
  : Job( parent ),
    d( new Private( this ) )
{
  d->ref = ref;
  d->state = Private::Begin;
}

ItemDeleteJob::~ ItemDeleteJob()
{
  delete d;
}

void ItemDeleteJob::doStart()
{
  TransactionBeginJob *begin = new TransactionBeginJob( this );
  addSubjob( begin );

  ItemStoreJob* store = new ItemStoreJob( d->ref, this );
  store->addFlag( "\\Deleted" );
  addSubjob( store );

  ExpungeJob *expunge = new ExpungeJob( this );
  addSubjob( expunge );

  TransactionCommitJob *commit = new TransactionCommitJob( this );
  connect( commit, SIGNAL(result(KJob*)), SLOT(jobDone(KJob*)) );
  addSubjob( commit );
}

#include "itemdeletejob.moc"
