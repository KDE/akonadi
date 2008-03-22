/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2007        Robert Zwerus <arzie@dds.nl>

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

#include "itemcreatejob.h"
#include "imapparser_p.h"
#include "itemserializer.h"

#include <kdebug.h>

using namespace Akonadi;

class Akonadi::ItemCreateJob::Private
{
  public:
    Private( ItemCreateJob *parent )
      : mParent( parent )
    {
    }

    ItemCreateJob *mParent;
    Collection collection;
    Item item;
    QStringList parts;
    Item::Id uid;
    QByteArray data;
};

ItemCreateJob::ItemCreateJob( const Item &item, const Collection &collection, QObject * parent ) :
    Job( parent ),
    d( new Private( this ) )
{
  Q_ASSERT( !item.mimeType().isEmpty() );
  d->item = item;
  d->parts = d->item.loadedPayloadParts();
  foreach ( const Attribute *attr, item.attributes() )
    d->parts << QString::fromLatin1( attr->type() );
  d->collection = collection;
}

ItemCreateJob::~ ItemCreateJob( )
{
  delete d;
}

void ItemCreateJob::doStart()
{
  QByteArray remoteId;

  if ( !d->item.remoteId().isEmpty() )
    remoteId = ' ' + ImapParser::quote( "\\RemoteId[" + d->item.remoteId().toUtf8() + ']' );
  // switch between a normal APPEND and a multipart X-AKAPPEND, based on the number of parts
  if ( d->parts.isEmpty() || (d->parts.size() == 1 && d->parts.first() == Item::PartBody) ) {
    if ( d->item.hasPayload() )
      ItemSerializer::serialize( d->item, Item::PartBody, d->data );
    int dataSize = d->data.size();

    writeData( newTag() + " APPEND " + QByteArray::number( d->collection.id() )
        + " (\\MimeType[" + d->item.mimeType().toLatin1() + ']' + remoteId + ") {"
        + QByteArray::number( dataSize ) + "}\n" );
  }
  else { // do a multipart X-AKAPPEND
    QByteArray command = newTag() + " X-AKAPPEND " + QByteArray::number( d->collection.id() )
        + " (\\MimeType[" + d->item.mimeType().toLatin1() + ']' + remoteId + ") ";

    QList<QByteArray> partSpecs;
    int totalSize = 0;
    foreach( const QString partName, d->parts ) {
      QByteArray partData;
      ItemSerializer::serialize( d->item, partName, partData );
      totalSize += partData.size();
      partSpecs.append( ImapParser::quote( partName.toLatin1() ) + ':' +
        QByteArray::number( partData.size() ) );
      d->data += partData;
    }
    command += '(' + ImapParser::join( partSpecs, "," ) + ") " +
      '{' + QByteArray::number( totalSize ) + "}\n";

    writeData( command );
  }
}

void ItemCreateJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == "+" ) { // ready for literal data
    writeData( d->data );
    if ( !d->data.endsWith( '\n' ) )
      writeData( "\n" );
    return;
  }
  if ( tag == this->tag() ) {
    if ( int pos = data.indexOf( "UIDNEXT" ) ) {
      bool ok = false;
      ImapParser::parseNumber( data, d->uid, &ok, pos + 7 );
      if ( !ok ) {
        kDebug( 5250 ) << "Invalid UIDNEXT response to APPEND command: "
                       << tag << data;
      }
    }
  }
}

Item ItemCreateJob::item() const
{
  if ( d->uid == 0 )
    return Item();
  Item item( d->item );
  item.setId( d->uid );
  return item;
}

#include "itemcreatejob.moc"
