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

#include <QtCore/QDebug>
#include <QtCore/QQueue>
#include <QtCore/QTimer>

using namespace Akonadi;

class Akonadi::JobQueuePrivate
{
  public:
    QQueue<Job*> queue;
    bool isStarted;
    bool jobRunning;
};

JobQueue::JobQueue( QObject * parent ) :
    Job( parent ),
    d( new JobQueuePrivate )
{
  d->isStarted = false;
  d->jobRunning = false;
  start();
}

JobQueue::~ JobQueue( )
{
  // TODO: what to do about still enqueued jobs?
  delete d;
}

void JobQueue::addSubJob( Job * job )
{
  Job::addSubJob( job );
  d->queue.enqueue( job );
  startNext();
}

void JobQueue::doStart( )
{
  d->isStarted = true;
  startNext();
}

void JobQueue::jobDone( Job * job )
{
  Q_UNUSED( job );
  d->jobRunning = false;
  startNext();
}

void JobQueue::startNext()
{
  QTimer::singleShot( 0, this, SLOT(slotStartNext()) );
}

void JobQueue::slotStartNext()
{
  if ( !d->isStarted || d->jobRunning || isEmpty() )
    return;
  d->jobRunning = true;
  Job *job = d->queue.dequeue();
  connect( job, SIGNAL(done(Akonadi::Job*)), SLOT(jobDone(Akonadi::Job*)) );
  job->start();
}

bool JobQueue::isEmpty( ) const
{
  return d->queue.isEmpty();
}

void JobQueue::start( )
{
  Job::start();
}

#include "jobqueue.moc"
