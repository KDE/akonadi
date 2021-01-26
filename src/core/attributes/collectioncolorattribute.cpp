/*
 *  SPDX-FileCopyrightText: 2015 Sandro Knauß <knauss@kolabsys.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "collectioncolorattribute.h"

#include <QByteArray>
#include <QString>

using namespace Akonadi;

CollectionColorAttribute::CollectionColorAttribute(const QColor &color)
    : mColor(color)
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
