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

#include <kdebug.h>

using namespace Akonadi;

class Akonadi::CollectionCreateJobPrivate {
  public:
    CollectionCreateJobPrivate() :
      cachePolicyId( -1 )
    {}

    Collection parent;
    QString name;
    QStringList contentTypes;
    Collection collection;
    QString remoteId;
    QList<QPair<QByteArray, QByteArray> > attributes;
    int cachePolicyId;
};

CollectionCreateJob::CollectionCreateJob( const Collection &parentCollection, const QString &name, QObject * parent ) :
    Job( parent ),
    d( new CollectionCreateJobPrivate )
{
  d->parent = parentCollection;
  d->name = name;
}

CollectionCreateJob::~ CollectionCreateJob( )
{
  delete d;
}

void CollectionCreateJob::doStart( )
{
  QByteArray command = newTag() + " CREATE \"" + d->name.toUtf8() + "\" ";
  command += QByteArray::number( d->parent.id() );
  command += " (";
  if ( !d->contentTypes.isEmpty() )
  {
    QList<QByteArray> cList;
    foreach( QString s, d->contentTypes ) cList << s.toLatin1();
    command += "MIMETYPE (" + ImapParser::join( cList, QByteArray(" ") ) + ')';
  }
  command += " REMOTEID \"" + d->remoteId.toUtf8() + '"';
  typedef QPair<QByteArray,QByteArray> QByteArrayPair;
  foreach ( const QByteArrayPair bp, d->attributes )
    command += ' ' + bp.first + ' ' + bp.second;
  if ( d->cachePolicyId > 0 )
    command += " CACHEPOLICY " + QByteArray::number( d->cachePolicyId );
  command += ")\n";
  writeData( command );
  emitWriteFinished();
}

void CollectionCreateJob::setContentTypes(const QStringList & contentTypes)
{
  d->contentTypes = contentTypes;
}

Collection CollectionCreateJob::collection() const
{
  return d->collection;
}

void CollectionCreateJob::doHandleResponse(const QByteArray & tag, const QByteArray & data)
{
  if ( tag == "*" ) {
    // TODO: share the parsing code with CollectionListJob
    int pos = 0;

    // collection and parent id
    int colId = -1;
    bool ok = false;
    pos = ImapParser::parseNumber( data, colId, &ok, pos );
    if ( !ok || colId <= 0 ) {
      kDebug( 5250 ) << "Could not parse collection id from response:" << data;
      return;
    }

    int parentId = -1;
    pos = ImapParser::parseNumber( data, parentId, &ok, pos );
    if ( !ok || parentId < 0 ) {
      kDebug( 5250 ) << "Could not parse parent id from response:" << data;
      return;
    }

    d->collection = Collection( colId );
    d->collection.setParent( parentId );
    d->collection.setName( d->name );
    d->collection.setRemoteId( d->remoteId );

  } else
    Job::doHandleResponse( tag, data );
}

void CollectionCreateJob::setRemoteId(const QString & remoteId)
{
  d->remoteId = remoteId;
}

void CollectionCreateJob::setAttribute(CollectionAttribute * attr)
{
  Q_ASSERT( !attr->type().isEmpty() );

  QByteArray value = ImapParser::quote( attr->toByteArray() );
  d->attributes.append( qMakePair( attr->type(), value ) );
}

void CollectionCreateJob::setCachePolicyId(int cachePolicyId)
{
  d->cachePolicyId = cachePolicyId;
}

#include "collectioncreatejob.moc"
