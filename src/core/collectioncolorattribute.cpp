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

#include "collectioncolorattribute.h"
#include "attributefactory.h"

#include <QByteArray>
#include <QString>

using namespace Akonadi;

CollectionColorAttribute::CollectionColorAttribute(const QColor &color)
    :  mColor(color)
{
}

void CollectionColorAttribute::setColor(const QColor &color)
{
    mColor = color;
}

QColor CollectionColorAttribute::color() const
{
    return mColor;
}

QByteArray CollectionColorAttribute::type() const
{
    return QByteArrayLiteral("collectioncolor");
}

CollectionColorAttribute *CollectionColorAttribute::clone() const
{
    return new CollectionColorAttribute(mColor);
}

QByteArray CollectionColorAttribute::serialized() const
{
    return mColor.isValid() ? mColor.name(QColor::HexArgb).toUtf8() : "";
}

void CollectionColorAttribute::deserialize(const QByteArray &data)
{
    mColor = QColor(QString::fromUtf8(data));
}
