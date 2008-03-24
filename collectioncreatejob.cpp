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
#include "imapparser_p.h"
#include "protocolhelper.h"

#include "job_p.h"

#include <kdebug.h>

using namespace Akonadi;

class Akonadi::CollectionCreateJobPrivate : public JobPrivate
{
  public:
    CollectionCreateJobPrivate( CollectionCreateJob *parent )
      : JobPrivate( parent )
    {
    }

    Collection mCollection;
};

CollectionCreateJob::CollectionCreateJob( const Collection &collection, QObject * parent )
  : Job( new CollectionCreateJobPrivate( this ), parent )
{
  Q_D( CollectionCreateJob );

  d->mCollection = collection;
}

CollectionCreateJob::~CollectionCreateJob( )
{
}

void CollectionCreateJob::doStart( )
{
  Q_D( CollectionCreateJob );

  QByteArray command = newTag() + " CREATE \"" + d->mCollection.name().toUtf8() + "\" ";
  command += QByteArray::number( d->mCollection.parent() );
  command += " (";
  if ( !d->mCollection.contentTypes().isEmpty() )
  {
    QList<QByteArray> cList;
    foreach( QString s, d->mCollection.contentTypes() ) cList << s.toLatin1();
    command += "MIMETYPE (" + ImapParser::join( cList, QByteArray(" ") ) + ')';
  }
  command += " REMOTEID \"" + d->mCollection.remoteId().toUtf8() + '"';
  foreach ( Attribute* attr, d->mCollection.attributes() )
    command += ' ' + attr->type() + ' ' + ImapParser::quote( attr->serialized() );
  command += ' ' + ProtocolHelper::cachePolicyToByteArray( d->mCollection.cachePolicy() );
  command += ")\n";
  writeData( command );
  emitWriteFinished();
}

Collection CollectionCreateJob::collection() const
{
  Q_D( const CollectionCreateJob );

  return d->mCollection;
}

void CollectionCreateJob::doHandleResponse(const QByteArray & tag, const QByteArray & data)
{
  Q_D( CollectionCreateJob );

  if ( tag == "*" ) {
    Collection col;
    ProtocolHelper::parseCollection( data, col );
    if ( !col.isValid() )
      return;

    col.setParent( d->mCollection.parent() );
    col.setName( d->mCollection.name() );
    col.setRemoteId( d->mCollection.remoteId() );
    d->mCollection = col;
  } else {
    Job::doHandleResponse( tag, data );
  }
}

#include "collectioncreatejob.moc"
