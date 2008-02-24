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

#include "collectionlistjob.h"

#include "imapparser.h"
#include "protocolhelper.h"

#include <kdebug.h>

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

using namespace Akonadi;

class Akonadi::CollectionListJobPrivate
{
  public:
    CollectionListJobPrivate( CollectionListJob *parent ) :
      q( parent ),
      emitTimer( new QTimer( parent ) ),
      unsubscribed( false )
    {
      emitTimer->setSingleShot( true );
      emitTimer->setInterval( 100 );
      QObject::connect( emitTimer, SIGNAL(timeout()), parent, SLOT(timeout()) );
      QObject::connect( parent, SIGNAL(result(KJob*)), parent, SLOT(timeout()) );
    }

    CollectionListJob *q;
    CollectionListJob::ListType type;
    Collection base;
    Collection::List baseList;
    Collection::List collections;
    QString resource;
    Collection::List pendingCollections;
    QTimer *emitTimer;
    bool unsubscribed;

    void timeout()
    {
      emitTimer->stop(); // in case we are called by result()
      if ( !pendingCollections.isEmpty() ) {
        emit q->collectionsReceived( pendingCollections );
        pendingCollections.clear();
      }
    }
};

CollectionListJob::CollectionListJob( const Collection &collection, ListType type, QObject *parent ) :
    Job( parent ),
    d( new CollectionListJobPrivate( this ) )
{
  Q_ASSERT( collection.isValid() );
  d->base = collection;
  d->type = type;
}

CollectionListJob::CollectionListJob(const Collection::List & cols, QObject * parent) :
    Job( parent ),
    d( new CollectionListJobPrivate( this ) )
{
  Q_ASSERT( !cols.isEmpty() );
  d->baseList = cols;
}

CollectionListJob::~CollectionListJob()
{
  delete d;
}

Collection::List CollectionListJob::collections() const
{
  return d->collections;
}

void CollectionListJob::doStart()
{
  if ( !d->baseList.isEmpty() ) {
    foreach ( const Collection col, d->baseList ) {
      new CollectionListJob( col, CollectionListJob::Local, this );
    }
    return;
  }

  QByteArray command = newTag();
  if ( d->unsubscribed )
    command += " X-AKLIST ";
  else
    command += " X-AKLSUB ";
  command += QByteArray::number( d->base.id() );
  command += ' ';
  switch ( d->type ) {
    case Local:
      command += "0 (";
      break;
    case Flat:
      command += "1 (";
      break;
    case Recursive:
      command += "INF (";
      break;
    default:
      Q_ASSERT( false );
  }

  if ( !d->resource.isEmpty() ) {
    command += "RESOURCE \"";
    command += d->resource.toUtf8();
    command += '"';
  }

  command += ")\n";
  writeData( command );
}

void CollectionListJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == "*" ) {
    Collection collection;
    ProtocolHelper::parseCollection( data, collection );
    if ( !collection.isValid() )
      return;

    d->collections.append( collection );
    d->pendingCollections.append( collection );
    if ( !d->emitTimer->isActive() )
      d->emitTimer->start();
    return;
  }
  kDebug( 5250 ) << "Unhandled server response" << tag << data;
}

void CollectionListJob::setResource(const QString & resource)
{
  d->resource = resource;
}

void CollectionListJob::slotResult(KJob * job)
{
  CollectionListJob *list = dynamic_cast<CollectionListJob*>( job );
  Q_ASSERT( job );
  d->collections += list->collections();
  Job::slotResult( job );
  if ( !job->error() && !hasSubjobs() )
    emitResult();
}

void CollectionListJob::includeUnsubscribed(bool include)
{
  d->unsubscribed = include;
}

#include "collectionlistjob.moc"
