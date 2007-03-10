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

#include "collectionattribute.h"
#include "collectionstatusjob.h"
#include "messagecollectionattribute.h"
#include "imapparser.h"

#include <QtCore/QDebug>

using namespace Akonadi;

class Akonadi::CollectionStatusJobPrivate
{
  public:
    Collection collection;
    CollectionAttribute::List attributes;
};

CollectionStatusJob::CollectionStatusJob( const Collection &collection, QObject * parent ) :
    Job( parent ),
    d( new CollectionStatusJobPrivate )
{
  d->collection = collection;
}

CollectionStatusJob::~ CollectionStatusJob( )
{
  delete d;
}

QList< CollectionAttribute * > CollectionStatusJob::attributes( ) const
{
  return d->attributes;
}

void CollectionStatusJob::doStart( )
{
  writeData( newTag() + " STATUS " + QByteArray::number( d->collection.id() ) + " (MESSAGES UNSEEN MIMETYPES)" );
}

void CollectionStatusJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == "*" ) {
    QByteArray token;
    int current = ImapParser::parseString( data, token );
    if ( token == "STATUS" ) {
      // folder path
      current = ImapParser::parseString( data, token, current );
      // result list
      QList<QByteArray> list;
      current = ImapParser::parseParenthesizedList( data, list, current );
      MessageCollectionAttribute *mcattr = new MessageCollectionAttribute();
      CollectionContentTypeAttribute *ctattr = new CollectionContentTypeAttribute();
      for ( int i = 0; i < list.count() - 1; i += 2 ) {
        if ( list[i] == "MESSAGES" ) {
          mcattr->setCount( list[i+1].toInt() );
        }
        else if ( list[i] == "UNSEEN" ) {
          mcattr->setUnreadCount( list[i+1].toInt() );
        } else if ( list[i] == "MIMETYPES" ) {
          QList<QByteArray> mimeTypes;
          ImapParser::parseParenthesizedList( list[i + 1], mimeTypes );
          ctattr->setContentTypes( mimeTypes );
        } else {
          qDebug() << "unknown STATUS response: " << list[i];
        }
      }
      d->attributes.append( mcattr );
      d->attributes.append( ctattr );
      foreach ( CollectionAttribute* attr, d->attributes )
        d->collection.addAttribute( attr );
      return;
    }
  }
  qDebug() << "unhandled response in collection status job: " << tag << data;
}

Collection CollectionStatusJob::collection() const
{
  return d->collection;
}


#include "collectionstatusjob.moc"
