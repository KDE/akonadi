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

using namespace PIM;

class PIM::ItemDeleteJobPrivate
{
  public:
    DataReference ref;
};

PIM::ItemDeleteJob::ItemDeleteJob(const DataReference & ref, QObject * parent) :
    Job( parent ),
    d( new ItemDeleteJobPrivate )
{
  d->ref = ref;
}

PIM::ItemDeleteJob::~ ItemDeleteJob()
{
  delete d;
}

void PIM::ItemDeleteJob::doStart()
{
  ItemStoreJob* job = new ItemStoreJob( d->ref, this );
  job->addFlag( "\\Deleted" );
  connect( job, SIGNAL(done(PIM::Job*)), SLOT(storeDone(PIM::Job*)) );
  job->start();
}

void PIM::ItemDeleteJob::storeDone(PIM::Job * job)
{
  if ( job->error() ) {
    setError( job->error(), job->errorMessage() );
    emit done( this );
  } else {
    job->deleteLater();
    ExpungeJob *ejob = new ExpungeJob( this );
    connect( ejob, SIGNAL(done(PIM::Job*)), SLOT(expungeDone(PIM::Job*)) );
    ejob->start();
  }
}

void PIM::ItemDeleteJob::expungeDone(PIM::Job * job)
{
  if ( job->error() )
    setError( job->error(), job->errorMessage() );
  job->deleteLater();
  emit done( this );
}

void PIM::ItemDeleteJob::doHandleResponse(const QByteArray & tag, const QByteArray & data)
{
  Q_UNUSED( tag );
  Q_UNUSED( data );
}

#include "itemdeletejob.moc"
