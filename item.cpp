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

#include "item.h"
#include "item_p.h"
#include "itemserializer_p.h"
#include "protocol_p.h"

#include <kurl.h>

#include <QtCore/QStringList>

using namespace Akonadi;

// Change to something != RFC822 as soon as the server supports it
const char* Item::FullPayload = "RFC822";

Item::Item()
  : Entity( new ItemPrivate )
{
}

Item::Item( Id id )
  : Entity( new ItemPrivate( id ) )
{
}

Item::Item( const QString & mimeType )
  : Entity( new ItemPrivate )
{
  d_func()->mMimeType = mimeType;
}

Item::Item( const Item &other )
  : Entity( other )
{
}

Item::~Item()
{
}

Item::Flags Item::flags() const
{
  return d_func()->mFlags;
}

void Item::setFlag( const QByteArray & name )
{
  Q_D( Item );
  d->mFlags.insert( name );
  if ( !d->mFlagsOverwritten )
    d->mAddedFlags.insert( name );
}

void Item::clearFlag( const QByteArray & name )
{
  Q_D( Item );
  d->mFlags.remove( name );
  if ( !d->mFlagsOverwritten )
    d->mDeletedFlags.insert( name );
}

void Item::setFlags( const Flags &flags )
{
  Q_D( Item );
  d->mFlags = flags;
  d->mFlagsOverwritten = true;
}

void Item::clearFlags()
{
  Q_D( Item );
  d->mFlags.clear();
  d->mFlagsOverwritten = true;
}

QDateTime Item::modificationTime() const
{
  return d_func()->mModificationTime;
}

void Item::setModificationTime( const QDateTime &datetime )
{
  d_func()->mModificationTime = datetime;
}

bool Item::hasFlag( const QByteArray & name ) const
{
  return d_func()->mFlags.contains( name );
}

QSet<QByteArray> Item::loadedPayloadParts() const
{
  return ItemSerializer::parts( *this );
}

QByteArray Item::payloadData() const
{
  int version = 0;
  QByteArray data;
  ItemSerializer::serialize( *this, FullPayload, data, version );
  return data;
}

void Item::setPayloadFromData( const QByteArray &data )
{
  ItemSerializer::deserialize( *this, FullPayload, data, 0, false );
}

int Item::revision() const
{
  return d_func()->mRevision;
}

void Item::setRevision( int rev )
{
  d_func()->mRevision = rev;
}

Entity::Id Item::collectionId() const
{
  return d_func()->mCollectionId;
}

void Item::setCollectionId( Entity::Id collectionId )
{
  d_func()->mCollectionId = collectionId;
}

QString Item::mimeType() const
{
  return d_func()->mMimeType;
}

void Item::setSize( qint64 size )
{
  Q_D( Item );
  d->mSize = size;
  d->mSizeChanged = true;
}

qint64 Item::size() const
{
  return d_func()->mSize;
}

void Item::setMimeType( const QString & mimeType )
{
  d_func()->mMimeType = mimeType;
}

bool Item::hasPayload() const
{
  return d_func()->mPayload != 0;
}

KUrl Item::url( UrlType type ) const
{
  KUrl url;
  url.setProtocol( QString::fromLatin1("akonadi") );
  url.addQueryItem( QLatin1String( "item" ), QString::number( id() ) );

  if ( type == UrlWithMimeType )
    url.addQueryItem( QLatin1String( "type" ), mimeType() );

  return url;
}

Item Item::fromUrl( const KUrl &url )
{
  if ( url.protocol() != QLatin1String( "akonadi" ) )
    return Item();

  const QString itemStr = url.queryItem( QLatin1String( "item" ) );
  bool ok = false;
  Item::Id itemId = itemStr.toLongLong( &ok );
  if ( !ok )
    return Item();

  return Item( itemId );
}

PayloadBase* Item::payloadBase() const
{
  return d_func()->mPayload;
}

void Item::setPayloadBase( PayloadBase* p )
{
  Q_D( Item );
  delete d->mPayload;
  d->mPayload = p;
}

AKONADI_DEFINE_PRIVATE( Item )
