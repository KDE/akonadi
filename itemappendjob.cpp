/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "itemappendjob.h"
#include "imapparser.h"
#include "itemstorejob.h"

#include <QtCore/QDebug>

using namespace Akonadi;

class Akonadi::ItemAppendJobPrivate
{
  public:
    Item item;
    Collection collection;
    QByteArray data;
    int uid;
    QByteArray tag;
};

ItemAppendJob::ItemAppendJob( const Item &item, const Collection &collection, QObject * parent ) :
    Job( parent ),
    d( new ItemAppendJobPrivate )
{
  Q_ASSERT( !item.mimeType().isEmpty() );
  d->item = item;
  d->collection = collection;
}

ItemAppendJob::~ ItemAppendJob( )
{
  delete d;
}

void ItemAppendJob::doStart()
{
  d->tag = newTag();
  newTag(); // prevent automatic response handling
  QByteArray remoteId;
  if ( !d->item.reference().remoteId().isEmpty() )
    remoteId = " \\RemoteId[" + d->item.reference().remoteId().toUtf8() + ']';
  // HACK we let ItemStoreJob take care of the other parts, it already can handle multi-part items.
  if ( d->item.availableParts().count() == 1 && d->item.availableParts().first() == Item::PartBody )
    d->data = d->item.part( Item::PartBody );
  writeData( d->tag + " APPEND " + QByteArray::number( d->collection.id() )
      + " (\\MimeType[" + d->item.mimeType().toLatin1() + ']' + remoteId + ") {"
      + QByteArray::number( d->data.size() ) + "}\n" );
}

void ItemAppendJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == "+" ) { // ready for literal data
    writeData( d->data );
    if ( !d->data.endsWith( '\n' ) )
      writeData( "\n" );
    return;
  }
  if ( tag == d->tag ) {
    if ( data.startsWith( "NO" ) || data.startsWith( "BAD" ) ) {
      setError( Unknown );
      setErrorText( QString::fromUtf8( data ) );
      emitResult();
      return;
    }
    if ( data.startsWith( "OK" ) ) {
      d->item.setReference( reference() );
      ItemStoreJob *store = new ItemStoreJob( d->item, this );
      store->storePayload();
      connect( store, SIGNAL(result(KJob*)), SLOT(storeResult(KJob*)) );
    }
    int pos = data.indexOf( "UIDNEXT" );
    bool ok = false;
    if ( pos > 0 )
      ImapParser::parseNumber( data, d->uid, &ok, pos + 7 );
    if ( !ok )
      qDebug() << "invalid response to item append job: " << tag << data;
  }
}

DataReference ItemAppendJob::reference() const
{
  if ( d->uid == 0 )
    return DataReference();
  return DataReference( d->uid, d->item.reference().remoteId() );
}

void ItemAppendJob::storeResult(KJob * job)
{
  if ( !job->error() )
    emitResult();
}

#include "itemappendjob.moc"
