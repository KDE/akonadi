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

#include <QtCore/QDebug>

using namespace Akonadi;

class Akonadi::CollectionCreateJobPrivate {
  public:
    Collection parent;
    QString name;
    QList<QByteArray> contentTypes;
    Collection collection;
    QString remoteId;
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
    command += "MIMETYPE (" + ImapParser::join( d->contentTypes, QByteArray(" ") ) + ')';
  command += " REMOTEID \"" + d->remoteId.toUtf8() + '"';
  command += ')';
  writeData( command );
}

void CollectionCreateJob::setContentTypes(const QList< QByteArray > & contentTypes)
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
      qDebug() << "could not parse response:" << data;
      return;
    }

    int parentId = -1;
    pos = ImapParser::parseNumber( data, parentId, &ok, pos );
    if ( !ok || parentId < 0 ) {
      qDebug() << "could not parse response:" << data;
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

#include "collectioncreatejob.moc"
