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

#include "collection.h"
#include "collectionlistjob.h"
#include "imapparser.h"

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

using namespace Akonadi;

class Akonadi::CollectionListJobPrivate
{
  public:
    CollectionListJob::ListType type;
    Collection base;
    Collection::List collections;
    QString resource;
};

CollectionListJob::CollectionListJob( const Collection &collection, ListType type, QObject *parent ) :
    Job( parent ),
    d( new CollectionListJobPrivate )
{
  Q_ASSERT( collection.isValid() );
  d->base = collection;
  d->type = type;
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
  QByteArray command = newTag() + " X-AKLIST ";
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

  command += ')';
  writeData( command );
}

void CollectionListJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == "*" ) {
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

    Collection collection( colId );
    collection.setParent( parentId );

    // attributes
    QList<QByteArray> attributes;
    pos = ImapParser::parseParenthesizedList( data, attributes, pos );

    for ( int i = 0; i < attributes.count() - 1; i += 2 ) {
      const QByteArray key = attributes.at( i );
      const QByteArray value = attributes.at( i + 1 );

      if ( key == "NAME" ) {
        collection.setName( QString::fromUtf8( value ) );
      } else if ( key == "REMOTEID" ) {
        collection.setRemoteId( QString::fromUtf8( value ) );
      } else if ( key == "MIMETYPE" ) {
        QList<QByteArray> list;
        ImapParser::parseParenthesizedList( value, list );
        collection.setContentTypes( list );
      } else {
        qDebug() << "Unknown collection attribute:" << key;
      }
    }

    // determine collection type
    if ( collection.parent() == Collection::root().id() ) {
      if ( collection.name() == Collection::searchFolder() )
        collection.setType( Collection::VirtualParent );
      else
        collection.setType( Collection::Resource );
    }
#warning Port me!
//    else if ( parentName == Collection::searchFolder() )
//       col->setType( Collection::Virtual );
    else {
      collection.setType( Collection::Folder );
    }

    d->collections.append( collection );
    return;
  }
  qDebug() << "unhandled server response in collection list job" << tag << data;
}

void CollectionListJob::setResource(const QString & resource)
{
  d->resource = resource;
}

#include "collectionlistjob.moc"
