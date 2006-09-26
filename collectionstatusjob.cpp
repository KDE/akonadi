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

#include "collectionattribute.h"
#include "collectionstatusjob.h"
#include "messagecollectionattribute.h"
#include "imapparser.h"

#include <QDebug>

using namespace PIM;

class PIM::CollectionStatusJobPrivate
{
  public:
    QString path;
    CollectionAttribute::List attributes;
};

PIM::CollectionStatusJob::CollectionStatusJob( const QString& path, QObject * parent ) :
    Job( parent ),
    d( new CollectionStatusJobPrivate )
{
  d->path = path;
}

PIM::CollectionStatusJob::~ CollectionStatusJob( )
{
  delete d;
}

QList< CollectionAttribute * > PIM::CollectionStatusJob::attributes( ) const
{
  return d->attributes;
}

void PIM::CollectionStatusJob::doStart( )
{
  writeData( newTag() + " STATUS \"" + d->path.toUtf8() + "\" (MESSAGES UNSEEN MIMETYPES)" );
}

void PIM::CollectionStatusJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
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
      return;
    }
  }
  qDebug() << "unhandled response in collection status job: " << tag << data;
}

QString PIM::CollectionStatusJob::path( ) const
{
  return d->path;
}


#include "collectionstatusjob.moc"
