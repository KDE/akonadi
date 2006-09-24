/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

using namespace PIM;

class PIM::ItemDeleteJobPrivate
{
  public:
    enum State {
      Begin,
      Store,
      Expunge,
      Commit
    };

    DataReference ref;
    State state;
};

PIM::ItemDeleteJob::ItemDeleteJob(const DataReference & ref, QObject * parent) :
    Job( parent ),
    d( new ItemDeleteJobPrivate )
{
  d->ref = ref;
  d->state = ItemDeleteJobPrivate::Begin;
}

PIM::ItemDeleteJob::~ ItemDeleteJob()
{
  delete d;
}

void PIM::ItemDeleteJob::doStart()
{
  TransactionBeginJob *begin = new TransactionBeginJob( this );
  connect( begin, SIGNAL(done(PIM::Job*)), SLOT(jobDone(PIM::Job*)) );
  begin->start();
}

void PIM::ItemDeleteJob::jobDone(PIM::Job * job)
{
  if ( job->error() ) {
    setError( job->error(), job->errorMessage() );
    job->deleteLater();
    emit done( this );
    return;
  }

  switch ( d->state ) {
    case ItemDeleteJobPrivate::Begin:
    {
      ItemStoreJob* store = new ItemStoreJob( d->ref, this );
      store->addFlag( "\\Deleted" );
      connect( store, SIGNAL(done(PIM::Job*)), SLOT(jobDone(PIM::Job*)) );
      store->start();
      d->state = ItemDeleteJobPrivate::Store;
      break;
    }
    case ItemDeleteJobPrivate::Store:
    {
      ExpungeJob *expunge = new ExpungeJob( this );
      connect( expunge, SIGNAL(done(PIM::Job*)), SLOT(jobDone(PIM::Job*)) );
      expunge->start();
      d->state = ItemDeleteJobPrivate::Expunge;
      break;
    }
    case ItemDeleteJobPrivate::Expunge:
    {
      TransactionCommitJob *commit = new TransactionCommitJob( this );
      connect( commit, SIGNAL(done(PIM::Job*)), SLOT(jobDone(PIM::Job*)) );
      commit->start();
      d->state = ItemDeleteJobPrivate::Commit;
      break;
    }
    case ItemDeleteJobPrivate::Commit:
      emit done( this );
      break;
  }

  job->deleteLater();
}

#include "itemdeletejob.moc"
