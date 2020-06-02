/*
 *  Copyright (C) 2015 Sandro Knauß <knauss@kolabsys.com>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

#ifndef AKONADI_COLLECTIONCOLORATTRIBUTE_H
#define AKONADI_COLLECTIONCOLORATTRIBUTE_H

#include "akonadicore_export.h"

#include <attribute.h>

#include <QColor>

namespace Akonadi
{

/**
 * @short Attribute that stores colors of a collection.
 *
 * Storing color in Akonadi makes it possible to sync them between client and server.
 *
 * @author Sandro Knauß <knauss@kolabsys.com>
 * @since 5.3
 */

class AKONADICORE_EXPORT CollectionColorAttribute : public Akonadi::Attribute
{
public:
    explicit CollectionColorAttribute() = default;
    explicit CollectionColorAttribute(const QColor &color);

    ~CollectionColorAttribute() override = default;

    void setColor(const QColor &color);
    QColor color() const;

    QByteArray type() const override;
    CollectionColorAttribute *clone() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    QColor mColor;
};

}

#endif
