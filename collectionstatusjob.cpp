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

#include "collectionstatusjob.h"
#include "imapparser_p.h"
#include "job_p.h"

#include <kdebug.h>

using namespace Akonadi;

class Akonadi::CollectionStatusJobPrivate : public JobPrivate
{
  public:
    CollectionStatusJobPrivate( CollectionStatusJob *parent )
      : JobPrivate( parent )
    {
    }

    Collection mCollection;
    CollectionStatus mStatus;
};

CollectionStatusJob::CollectionStatusJob( const Collection &collection, QObject * parent )
  : Job( new CollectionStatusJobPrivate( this ), parent )
{
  Q_D( CollectionStatusJob );

  d->mCollection = collection;
}

CollectionStatusJob::~ CollectionStatusJob( )
{
  Q_D( CollectionStatusJob );

  delete d;
}

void CollectionStatusJob::doStart( )
{
  Q_D( CollectionStatusJob );

  writeData( newTag() + " STATUS " + QByteArray::number( d->mCollection.id() ) + " (MESSAGES UNSEEN)\n" );
}

void CollectionStatusJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  Q_D( CollectionStatusJob );

  if ( tag == "*" ) {
    QByteArray token;
    int current = ImapParser::parseString( data, token );
    if ( token == "STATUS" ) {
      // folder path
      current = ImapParser::parseString( data, token, current );
      // result list
      QList<QByteArray> list;
      current = ImapParser::parseParenthesizedList( data, list, current );
      for ( int i = 0; i < list.count() - 1; i += 2 ) {
        if ( list[i] == "MESSAGES" ) {
          d->mStatus.setCount( list[i+1].toInt() );
        } else if ( list[i] == "UNSEEN" ) {
          d->mStatus.setUnreadCount( list[i+1].toInt() );
        } else {
          kDebug( 5250 ) << "Unknown STATUS response: " << list[i];
        }
      }

      d->mCollection.setStatus( d->mStatus );
      return;
    }
  }
  kDebug( 5250 ) << "Unhandled response: " << tag << data;
}

Collection CollectionStatusJob::collection() const
{
  Q_D( const CollectionStatusJob );

  return d->mCollection;
}

CollectionStatus Akonadi::CollectionStatusJob::status() const
{
  Q_D( const CollectionStatusJob );

  return d->mStatus;
}

#include "collectionstatusjob.moc"
