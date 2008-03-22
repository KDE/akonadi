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

#include "item.h"
#include "item_p.h"
#include "itemserializer.h"
#include "protocol_p.h"

using namespace Akonadi;

const QLatin1String Item::PartBody = QLatin1String( AKONADI_PART_BODY );
const QLatin1String Item::PartEnvelope = QLatin1String( "ENVELOPE" );
const QLatin1String Item::PartHeader = QLatin1String( "HEAD" );

Item::Item()
  : Entity( new ItemPrivate ),
    m_payload( 0 )
{
}

Item::Item( Id id )
  : Entity( new ItemPrivate( id ) ),
    m_payload( 0 )
{
}

Item::Item( const QString & mimeType )
  : Entity( new ItemPrivate ),
    m_payload( 0 )
{
  d_func()->mMimeType = mimeType;
}

Item::Item( const Item &other )
  : Entity( other ),
    m_payload( 0 )
{
  if ( other.m_payload )
    m_payload = other.m_payload->clone();
  else
    m_payload = 0;
}

Item::~Item()
{
  delete m_payload;
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

void Item::removePart( const QString &identifier )
{
  removeRawPart( identifier );
}

QByteArray Item::part( const QString &identifier ) const
{
  QByteArray data;
  ItemSerializer::serialize( *this, identifier, data );
  return data;
}

QStringList Item::availableParts() const
{
  QStringList payloadParts = ItemSerializer::parts( *this );
  return payloadParts + d_func()->mParts.keys();
}

int Item::rev() const
{
  return d_func()->mRevision;
}

void Item::setRev( const int rev )
{
  d_func()->mRevision = rev;
}

void Item::incRev()
{
  d_func()->mRevision = d_func()->mRevision + 1;
}

QString Item::mimeType() const
{
  return d_func()->mMimeType;
}

void Item::setMimeType( const QString & mimeType )
{
  d_func()->mMimeType = mimeType;
}

Item& Item::operator=( const Item & other )
{
  if ( this != &other ) {
    Entity::operator=( other );
    if ( other.m_payload )
      m_payload = other.m_payload->clone();
    else
      m_payload = 0;
  }

  return *this;
}

bool Item::hasPayload() const
{
  return m_payload != 0;
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
  return Item( url.queryItems()[ QString::fromLatin1("item") ].toLongLong() );
}

bool Item::urlIsValid( const KUrl &url )
{
  return url.protocol() == QString::fromLatin1("akonadi") && url.queryItems().contains( QString::fromLatin1("item") );
}

void Item::addRawPart( const QString & label, const QByteArray & data )
{
  d_func()->mParts.insert( label, data );
}

void Item::removeRawPart( const QString & label )
{
  d_func()->mParts.remove( label );
}

QByteArray Item::rawPart( const QString & label ) const
{
  return d_func()->mParts.value( label );
}

AKONADI_DEFINE_PRIVATE( Item )
