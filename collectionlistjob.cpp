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
#include <QStringList>
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
    QList<QByteArray> attributes = data.mid( begin + 1, end - begin - 1 ).split( ' ' );
    begin = data.indexOf( '"', end );
    char delim = data[begin + 1];
    begin = data.indexOf( ' ', begin + 2 );

    QByteArray folderName;
    if ( data[begin + 1] == '"' )
      folderName = data.mid( begin + 2, data.lastIndexOf( '"' ) - begin - 2 );
    else
      folderName = data.mid( begin + 1 ); // ### strip trailing newline?
    // strip trailing delimiters
    if ( folderName.endsWith( delim ) )
      folderName.truncate( data.length() - 1 );

    QByteArray parentName = folderName.mid( 0, folderName.lastIndexOf( delim ) + 1 );
    // strip trailing delimiter, but not if this is root
    if ( parentName.endsWith( Collection::delimiter() ) && parentName != Collection::root() )
      parentName.truncate( parentName.length() - 1 );
    Collection *col = new Collection( folderName );
    col->setParent( parentName );

    // determine collection type, TODO: search folder
    if ( parentName == Collection::root() ) {
      if ( folderName == Collection::searchFolder() )
        col->setType( Collection::VirtualParent );
      else
        col->setType( Collection::Resource );
    } else if ( parentName == Collection::searchFolder() )
      col->setType( Collection::Virtual );
    else
      col->setType( Collection::Folder );

    QStringList contentTypes;
    if ( !attributes.contains( "\\Noinferiors" ) )
      contentTypes << "akonadi/folder";
    QByteArray mimetypes;
    foreach ( QByteArray ba, attributes ) {
      if ( ba.startsWith( "\\MimeTypes" ) ) {
        mimetypes = ba;
        break;
      }
    }
    if ( !mimetypes.isEmpty() ) {
      int begin = mimetypes.indexOf( '[' );
      int end = mimetypes.lastIndexOf( ']' );
      QList<QByteArray> l = mimetypes.mid( begin + 1, end - begin - 1 ).split( ',' );
      foreach ( QByteArray type, l )
        contentTypes << QString::fromLatin1( type );
    }
    col->setContentTypes( contentTypes );

    d->collections.append( col );
    qDebug() << "received list response: delim: " << delim << " name: " << folderName << "attrs: " << attributes << " parent: " << parentName;
  }
  qDebug() << "unhandled server response in collection list job" << tag << data;
}

#include "collectionlistjob.moc"
