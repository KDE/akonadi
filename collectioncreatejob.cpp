/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "collectioncreatejob.h"
#include "imapparser.h"
#include "protocolhelper.h"

#include <kdebug.h>

using namespace Akonadi;

class Akonadi::CollectionCreateJobPrivate {
  public:
    CollectionCreateJobPrivate()
    {}

    Collection collection;
};

CollectionCreateJob::CollectionCreateJob( const Collection &collection, QObject * parent ) :
    Job( parent ),
    d( new CollectionCreateJobPrivate )
{
  d->collection = collection;
}

CollectionCreateJob::~ CollectionCreateJob( )
{
  delete d;
}

void CollectionCreateJob::doStart( )
{
  QByteArray command = newTag() + " CREATE \"" + d->collection.name().toUtf8() + "\" ";
  command += QByteArray::number( d->collection.parent() );
  command += " (";
  if ( !d->collection.contentTypes().isEmpty() )
  {
    QList<QByteArray> cList;
    foreach( QString s, d->collection.contentTypes() ) cList << s.toLatin1();
    command += "MIMETYPE (" + ImapParser::join( cList, QByteArray(" ") ) + ')';
  }
  command += " REMOTEID \"" + d->collection.remoteId().toUtf8() + '"';
  foreach ( CollectionAttribute* attr, d->collection.attributes() )
    command += ' ' + attr->type() + ' ' + ImapParser::quote( attr->toByteArray() );
  command += ' ' + ProtocolHelper::cachePolicyToByteArray( d->collection.cachePolicy() );
  command += ")\n";
  writeData( command );
  emitWriteFinished();
}

Collection CollectionCreateJob::collection() const
{
  return d->collection;
}

void CollectionCreateJob::doHandleResponse(const QByteArray & tag, const QByteArray & data)
{
  if ( tag == "*" ) {
    Collection col;
    ProtocolHelper::parseCollection( data, col );
    if ( !col.isValid() )
      return;

    col.setParent( d->collection.parent() );
    col.setName( d->collection.name() );
    col.setRemoteId( d->collection.remoteId() );
    d->collection = col;
  } else {
    Job::doHandleResponse( tag, data );
  }
}

#include "collectioncreatejob.moc"
