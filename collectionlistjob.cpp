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
#include "imapparser.h"

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

using namespace Akonadi;

class Akonadi::CollectionListJobPrivate
{
  public:
    bool recursive;
    QString prefix;
    Collection::List collections;
    QString resource;
};

CollectionListJob::CollectionListJob( const QString &prefix, bool recursive, QObject *parent ) :
    Job( parent ),
    d( new CollectionListJobPrivate )
{
  d->prefix = prefix;
  d->recursive = recursive;
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
  QByteArray command = newTag() + " LIST \"";
  if ( !d->resource.isEmpty() )
    command += '#' + d->resource.toUtf8();
  command += "\" \"" + d->prefix.toUtf8();
  if ( !d->prefix.endsWith( Collection::delimiter() ) && !d->prefix.isEmpty() )
    command += Collection::delimiter().toUtf8();
  command += ( d->recursive ? '*' : '%' );
  command += '\"';
  writeData( command );
}

void CollectionListJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == "*" && data.startsWith( "LIST" ) ) {
    int current = 4; // 'LIST'

    // attributes
    QList<QByteArray> attributes;
    current = ImapParser::parseParenthesizedList( data, attributes, current );

    // delimiter
    QByteArray delim;
    current = ImapParser::parseString( data, delim, current );
    Q_ASSERT( delim.length() == 1 );

    // collection name
    QString folderName;
    current = ImapParser::parseString( data, folderName, current );

    // strip trailing delimiters
    if ( folderName.endsWith( QString::fromUtf8( delim ) ) )
      folderName.truncate( data.length() - 1 );

    QString parentName = folderName.mid( 0, folderName.lastIndexOf( QString::fromUtf8( delim ) ) + 1 );
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

    QList<QByteArray> contentTypes;
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
      contentTypes = mimetypes.mid( begin + 1, end - begin - 1 ).split( ',' );
    }
    col->setContentTypes( contentTypes );

    d->collections.append( col );
    qDebug() << "received list response: delim: " << delim << " name: " << folderName << "attrs: " << attributes << " parent: " << parentName;
    return;
  }
  qDebug() << "unhandled server response in collection list job" << tag << data;
}

void CollectionListJob::setResource(const QString & resource)
{
  d->resource = resource;
}

#include "collectionlistjob.moc"
