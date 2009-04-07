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

#include "collectionfetchjob.h"

#include "imapparser_p.h"
#include "job_p.h"
#include "protocol_p.h"
#include "protocolhelper.h"
#include "entity_p.h"

#include <kdebug.h>

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

using namespace Akonadi;

class Akonadi::CollectionFetchJobPrivate : public JobPrivate
{
  public:
    CollectionFetchJobPrivate( CollectionFetchJob *parent )
      : JobPrivate( parent ),
        mUnsubscribed( false )
    {
    }

    Q_DECLARE_PUBLIC( CollectionFetchJob )

    CollectionFetchJob::Type mType;
    Collection mBase;
    Collection::List mBaseList;
    Collection::List mCollections;
    QString mResource;
    Collection::List mPendingCollections;
    QTimer *mEmitTimer;
    bool mUnsubscribed;

    void timeout()
    {
      Q_Q( CollectionFetchJob );

      mEmitTimer->stop(); // in case we are called by result()
      if ( !mPendingCollections.isEmpty() ) {
        emit q->collectionsReceived( mPendingCollections );
        mPendingCollections.clear();
      }
    }
};

CollectionFetchJob::CollectionFetchJob( const Collection &collection, Type type, QObject *parent )
  : Job( new CollectionFetchJobPrivate( this ), parent )
{
  Q_D( CollectionFetchJob );

  d->mBase = collection;
  d->mType = type;

  d->mEmitTimer = new QTimer( this );
  d->mEmitTimer->setSingleShot( true );
  d->mEmitTimer->setInterval( 100 );
  connect( d->mEmitTimer, SIGNAL(timeout()), this, SLOT(timeout()) );
  connect( this, SIGNAL(result(KJob*)), this, SLOT(timeout()) );
}

CollectionFetchJob::CollectionFetchJob( const Collection::List & cols, QObject * parent )
  : Job( new CollectionFetchJobPrivate( this ), parent )
{
  Q_D( CollectionFetchJob );

  Q_ASSERT( !cols.isEmpty() );
  d->mBaseList = cols;

  d->mEmitTimer = new QTimer( this );
  d->mEmitTimer->setSingleShot( true );
  d->mEmitTimer->setInterval( 100 );
  connect( d->mEmitTimer, SIGNAL(timeout()), this, SLOT(timeout()) );
  connect( this, SIGNAL(result(KJob*)), this, SLOT(timeout()) );
}

CollectionFetchJob::~CollectionFetchJob()
{
}

Collection::List CollectionFetchJob::collections() const
{
  Q_D( const CollectionFetchJob );

  return d->mCollections;
}

void CollectionFetchJob::doStart()
{
  Q_D( CollectionFetchJob );

  if ( !d->mBaseList.isEmpty() ) {
    foreach ( const Collection &col, d->mBaseList ) {
      new CollectionFetchJob( col, CollectionFetchJob::Base, this );
    }
    return;
  }

  if ( !d->mBase.isValid() && d->mBase.remoteId().isEmpty() ) {
    setError( Unknown );
    setErrorText( QLatin1String( "Invalid collection given." ) );
    emitResult();
    return;
  }

  QByteArray command = d->newTag();
  if ( !d->mBase.isValid() )
    command += " " AKONADI_CMD_RID;
  if ( d->mUnsubscribed )
    command += " X-AKLIST ";
  else
    command += " X-AKLSUB ";
  if ( d->mBase.isValid() )
    command += QByteArray::number( d->mBase.id() );
  else
    command += ImapParser::quote( d->mBase.remoteId().toUtf8() );
  command += ' ';
  switch ( d->mType ) {
    case Base:
      command += "0 (";
      break;
    case FirstLevel:
      command += "1 (";
      break;
    case Recursive:
      command += "INF (";
      break;
    default:
      Q_ASSERT( false );
  }

  if ( !d->mResource.isEmpty() ) {
    command += "RESOURCE \"";
    command += d->mResource.toUtf8();
    command += '"';
  }

  command += ")\n";
  d->writeData( command );
}

void CollectionFetchJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  Q_D( CollectionFetchJob );

  if ( tag == "*" ) {
    Collection collection;
    ProtocolHelper::parseCollection( data, collection );
    if ( !collection.isValid() )
      return;

    collection.d_ptr->resetChangeLog();
    d->mCollections.append( collection );
    d->mPendingCollections.append( collection );
    if ( !d->mEmitTimer->isActive() )
      d->mEmitTimer->start();
    return;
  }
  kDebug( 5250 ) << "Unhandled server response" << tag << data;
}

void CollectionFetchJob::setResource(const QString & resource)
{
  Q_D( CollectionFetchJob );

  d->mResource = resource;
}

void CollectionFetchJob::slotResult(KJob * job)
{
  Q_D( CollectionFetchJob );

  CollectionFetchJob *list = dynamic_cast<CollectionFetchJob*>( job );
  Q_ASSERT( job );
  d->mCollections += list->collections();
  Job::slotResult( job );
  if ( !job->error() && !hasSubjobs() )
    emitResult();
}

void CollectionFetchJob::includeUnsubscribed(bool include)
{
  Q_D( CollectionFetchJob );

  d->mUnsubscribed = include;
}

#include "collectionfetchjob.moc"
