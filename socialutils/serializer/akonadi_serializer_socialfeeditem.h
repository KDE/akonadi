/*
  Social feed serializer
  Copyright (C) 2012  Martin Klapetek <martin.klapetek@gmail.com>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or (at your
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

#ifndef AKONADI_SERIALIZER_SOCIALFEEDITEMSERIALIZER_H
#define AKONADI_SERIALIZER_SOCIALFEEDITEMSERIALIZER_H

#include <QtCore/QObject>

#include "akonadi/itemserializerplugin.h"

namespace Akonadi {

class SocialFeedItemSerializerPlugin : public QObject, public ItemSerializerPlugin
{
  Q_OBJECT
  Q_INTERFACES( Akonadi::ItemSerializerPlugin )

  public:
    bool deserialize( Item &item, const QByteArray &label, QIODevice &data, int version );
    void serialize( const Item &item, const QByteArray &label, QIODevice &data, int &version );
    QSet<QByteArray> parts( const Item &item ) const;
};

}

#endif
