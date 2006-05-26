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

#include "collection.h"
#include "collectionselectjob.h"
#include "messagefetchjob.h"

#include <QDebug>

using namespace PIM;

class PIM::MessageFetchJobPrivate
{
  public:
    QByteArray path;
    DataReference uid;
    Message::List messages;
    QByteArray tag;
};

PIM::MessageFetchJob::MessageFetchJob( const QByteArray & path, QObject * parent ) :
    Job( parent ),
    d( new MessageFetchJobPrivate )
{
  d->path = path;
}

PIM::MessageFetchJob::MessageFetchJob( const DataReference & ref, QObject * parent ) :
    Job( parent ),
    d( new MessageFetchJobPrivate )
{
  d->path = Collection::root();
  d->uid = ref;
}

PIM::MessageFetchJob::~ MessageFetchJob( )
{
  delete d;
}

void PIM::MessageFetchJob::doStart()
{
  CollectionSelectJob *job = new CollectionSelectJob( d->uid.isNull() ? d->path : Collection::root(), this );
  connect( job, SIGNAL(done(PIM::Job*)), SLOT(selectDone(PIM::Job*)) );
  job->start();
}

void PIM::MessageFetchJob::handleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == d->tag ) {
    if ( !data.startsWith( "OK" ) )
      setError( Unknown );
    emit done( this );
    return;
  }
  qDebug() << "Unhandled response in message fetch job: " << tag << data;
}

Message::List PIM::MessageFetchJob::messages() const
{
  return d->messages;
}

void PIM::MessageFetchJob::selectDone( PIM::Job * job )
{
  job->deleteLater();
  // the collection is now selected, fetch the message(s)
  d->tag = newTag();
  QByteArray command = d->tag + " FETCH ";
  if ( d->uid.isNull() )
    command += "0:* ENVELOPE";
  else
    command += d->uid.persistanceID().toLatin1() + " (BODY[])";
  writeData( command );
}

#include "messagefetchjob.moc"
