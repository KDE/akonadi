/*
    Copyright 2017 Daniel Vrátil <dvratil@kde.org>

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

#ifndef AKONADI_CORE_FAVORITECOLLECTIONATTRIBUTE_H_
#define AKONADI_CORE_FAVORITECOLLECTIONATTRIBUTE_H_

#include "attribute.h"
#include "akonadicore_export.h"

namespace Akonadi {

class AKONADICORE_EXPORT FavoriteCollectionAttribute : public Attribute
{
public:
    explicit FavoriteCollectionAttribute() = default;

    Attribute *clone() const override;
    QByteArray type() const override;

    void deserialize(const QByteArray &data) override;
    QByteArray serialized() const override;
};

}

#endif
