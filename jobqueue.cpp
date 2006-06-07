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

#include "jobqueue.h"

#include <QDebug>
#include <QQueue>

using namespace PIM;

class PIM::JobQueuePrivate
{
  public:
    QQueue<Job*> queue;
    bool isStarted;
    bool jobRunning;
};

PIM::JobQueue::JobQueue( QObject * parent ) :
    Job( parent ),
    d( new JobQueuePrivate )
{
  d->isStarted = false;
  d->jobRunning = false;
  start();
}

PIM::JobQueue::~ JobQueue( )
{
  // TODO: what to do about still enqueued jobs?
  delete d;
}

void PIM::JobQueue::addJob( PIM::Job * job )
{
  d->queue.enqueue( job );
  startNext();
}

void PIM::JobQueue::doStart( )
{
  d->isStarted = true;
  startNext();
}

void PIM::JobQueue::jobDone( PIM::Job * job )
{
  Q_UNUSED( job );
  d->jobRunning = false;
  startNext();
}

void PIM::JobQueue::startNext( )
{
  if ( !d->isStarted || d->jobRunning || isEmpty() )
    return;
  d->jobRunning = true;
  Job *job = d->queue.dequeue();
  connect( job, SIGNAL(done(PIM::Job*)), SLOT(jobDone(PIM::Job*)) );
  job->start();
}

bool PIM::JobQueue::isEmpty( ) const
{
  return d->queue.isEmpty();
}

void PIM::JobQueue::start( )
{
  Job::start();
}

#include "jobqueue.moc"
