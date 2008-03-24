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

#include <kurl.h>

#include <QtCore/QStringList>

#include "item.h"
#include "item_p.h"
#include "itemserializer.h"
#include "protocol_p.h"

using namespace Akonadi;

const QLatin1String Item::PartBody = QLatin1String( AKONADI_PART_BODY );
const QLatin1String Item::PartEnvelope = QLatin1String( "ENVELOPE" );
const QLatin1String Item::PartHeader = QLatin1String( "HEAD" );

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
  d_func()->mFlags.insert( name );
}

void Item::unsetFlag( const QByteArray & name )
{
  d_func()->mFlags.remove( name );
}

bool Item::hasFlag( const QByteArray & name ) const
{
  return d_func()->mFlags.contains( name );
}

void Item::addPart( const QString &identifier, const QByteArray &data )
{
  ItemSerializer::deserialize( *this, identifier, data );
}

QByteArray Item::part( const QString &identifier ) const
{
  QByteArray data;
  ItemSerializer::serialize( *this, identifier, data );
  return data;
}

QStringList Item::loadedPayloadParts() const
{
  return ItemSerializer::parts( *this );
}

QStringList Item::availableParts() const
{
  QStringList payloadParts = ItemSerializer::parts( *this );
  foreach ( const Attribute *attr, attributes() )
    payloadParts << QString::fromLatin1( attr->type() );
  return payloadParts;
}

int Item::revision() const
{
  return d_func()->mRevision;
}

void Item::setRevision( const int rev )
{
  d_func()->mRevision = rev;
}

QString Item::mimeType() const
{
  return d_func()->mMimeType;
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
