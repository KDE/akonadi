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

#ifndef AKONADI_SOCIALUTILS_SOCIALNETWORKATTRIBUTES_H
#define AKONADI_SOCIALUTILS_SOCIALNETWORKATTRIBUTES_H

#include "libakonadisocialutils_export.h"

#include "akonadi/attribute.h"

#include <QString>
#include <QVariantMap>

namespace Akonadi {

class SocialNetworkAttributesPrivate;

class LIBAKONADISOCIALUTILS_EXPORT SocialNetworkAttributes : public Akonadi::Attribute
{
  public:
    SocialNetworkAttributes();

    /**
     * Constructs the attribute
     * @param userName User that is signed in for this collection/resource
     * @param networkName Name of the network this collection belongs to (eg. Facebook, Twitter etc)
     * @param canPublish True if we can publish stuff through this resource
     * @param maxPostLength Max length of the post that can be published (eg. 140 for Twitter)
     */
    SocialNetworkAttributes( const QString &userName, const QString &networkName,
                             bool canPublish, uint maxPostLength );
    ~SocialNetworkAttributes();

    void deserialize( const QByteArray &data );
    QByteArray serialized() const;
    Akonadi::Attribute *clone() const;
    QByteArray type() const;

    /**
     * Returns the userName of the user that is signed in with this resource
     */
    QString userName() const;

    /**
     * Returns the network name this collection belongs to (eg. Facebook, Twitter etc)
     */
    QString networkName() const;

    /**
     * Returns true if this network can be published to, false otherwise
     */
    bool canPublish() const;

    /**
     * Returns max length of the post that can be published (eg. 140 chars for Twitter)
     */
    uint maxPostLength() const;

  private:
    SocialNetworkAttributesPrivate * const d_ptr;
    Q_DECLARE_PRIVATE( SocialNetworkAttributes )
};

}

#endif // SOCIALNETWORKATTRIBUTES_H
