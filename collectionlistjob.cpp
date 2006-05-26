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

#include "collection.h"
#include "collectionlistjob.h"

#include <QDebug>
#include <QHash>
#include <QTimer>

using namespace PIM;

class PIM::CollectionListJobPrivate
{
  public:
    bool recursive;
    QByteArray prefix;
    QByteArray tag;
    Collection::List collections;
};

PIM::CollectionListJob::CollectionListJob( const QByteArray &prefix, bool recursive, QObject *parent ) :
    Job( parent ),
    d( new CollectionListJobPrivate )
{
  d->prefix = prefix;
  d->recursive = recursive;
}

PIM::CollectionListJob::~CollectionListJob()
{
  delete d;
}

PIM::Collection::List PIM::CollectionListJob::collections() const
{
  return d->collections;
}

void PIM::CollectionListJob::doStart()
{
  d->tag = newTag();
  writeData( d->tag + " LIST \"" + d->prefix + "\" " + ( d->recursive ? '*' : '%' ) );
}

void PIM::CollectionListJob::handleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == d->tag ) {
    if ( !data.startsWith( "OK" ) )
      setError( Unknown );
    emit done( this );
    return;
  }
  if ( tag == "*" && data.startsWith( "LIST" ) ) {
    int begin = data.indexOf( '(' );
    int end = data.indexOf( ')' );
    QList<QByteArray> attributes = data.mid( begin, end - begin - 1 ).split( ' ' );
    begin = data.indexOf( '"', end );
    char delim = data[begin + 1];
    begin = data.indexOf( ' ', begin + 2 );
    QByteArray folderName;
    if ( data[begin + 1] == '"' ) // ### are quoted results allowed?
      folderName = d->prefix + data.mid( begin + 2, data.length() - begin - 3 );
    else
      folderName = d->prefix + data.mid( begin + 1 );
    QByteArray parentName = folderName.mid( 0, folderName.lastIndexOf( delim ) + 1 ); // ### handle trailing delimiters!
    Collection *col = new Collection( folderName );
    col->setParent( parentName );
    // determine collection type, TODO: search folder
    if ( parentName == Collection::root() )
      col->setType( Collection::Resource );
    else
      col->setType( Collection::Folder );
    // TODO: add other collection parameters
    d->collections.append( col );
    qDebug() << "received list response: delim: " << delim << " name: " << folderName << "attrs: " << attributes << " parent: " << parentName;
  }
  qDebug() << "unhandled server response in collection list job" << tag << data;
}

#include "collectionlistjob.moc"
