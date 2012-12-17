/*
  Copyright (C) 2012  Martin Klapetek <martin.klapetek@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "socialnetworkattributes.h"

#include "akonadi/config-akonadi.h"

#include <QString>

#include <qjson/serializer.h>
#include <qjson/parser.h>

class Akonadi::SocialNetworkAttributesPrivate
{
  public:
    QVariantMap attributes;
};

Akonadi::SocialNetworkAttributes::SocialNetworkAttributes()
  : d_ptr( new SocialNetworkAttributesPrivate() )
{
}

Akonadi::SocialNetworkAttributes::SocialNetworkAttributes( const QString &userName,
                                                           const QString &networkName,
                                                           bool canPublish,
                                                           uint maxPostLength )
  : d_ptr( new SocialNetworkAttributesPrivate() )
{
  Q_D( SocialNetworkAttributes );

  d->attributes[QLatin1String( "userName" )] = userName;
  d->attributes[QLatin1String( "networkName" )] = networkName;
  d->attributes[QLatin1String( "canPublish" )] = canPublish;
  d->attributes[QLatin1String( "maxPostLength" )] = maxPostLength;
}

Akonadi::SocialNetworkAttributes::~SocialNetworkAttributes()
{
}

void Akonadi::SocialNetworkAttributes::deserialize( const QByteArray &data )
{
  Q_D( SocialNetworkAttributes );

  QJson::Parser parser;
  d->attributes = parser.parse(data).toMap();
}

QByteArray Akonadi::SocialNetworkAttributes::serialized() const
{
  Q_D( const SocialNetworkAttributes );

  QJson::Serializer serializer;
#if !defined( USE_QJSON_0_8 )
  return serializer.serialize( d->attributes );
#else
  return serializer.serialize( d->attributes, 0 );
#endif
}

Akonadi::Attribute *Akonadi::SocialNetworkAttributes::clone() const
{
  Q_D( const SocialNetworkAttributes );

  return
    new SocialNetworkAttributes(
      d->attributes[QLatin1String( "userName" )].toString(),
      d->attributes[QLatin1String( "networkName" )].toString(),
      d->attributes[QLatin1String( "canPublish" )].toBool(),
      d->attributes[QLatin1String( "maxPostLength" )].toUInt() );
}

QByteArray Akonadi::SocialNetworkAttributes::type() const
{
  return QByteArray( "socialattributes" );
}

QString Akonadi::SocialNetworkAttributes::userName() const
{
  Q_D( const SocialNetworkAttributes );

  return d->attributes[QLatin1String( "userName" )].toString();
}

QString Akonadi::SocialNetworkAttributes::networkName() const
{
  Q_D( const SocialNetworkAttributes );

  return d->attributes[QLatin1String( "networkName" )].toString();
}

bool Akonadi::SocialNetworkAttributes::canPublish() const
{
  Q_D( const SocialNetworkAttributes );

  return d->attributes[QLatin1String( "canPublish" )].toBool();
}

uint Akonadi::SocialNetworkAttributes::maxPostLength() const
{
  Q_D( const SocialNetworkAttributes );

  return d->attributes[QLatin1String( "maxPostLength" )].toUInt();
}
