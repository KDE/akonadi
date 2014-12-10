/*
    Copyright (C) 2009 Kevin Ottens <ervin@kde.org>

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

#include "collectionquotaattribute.h"

#include <QtCore/QByteArray>

using namespace Akonadi;

class CollectionQuotaAttribute::Private
{
public:
    Private(qint64 currentValue, qint64 maxValue)
        : mCurrentValue(currentValue)
        , mMaximumValue(maxValue)
    {
    }

    qint64 mCurrentValue;
    qint64 mMaximumValue;
};

CollectionQuotaAttribute::CollectionQuotaAttribute()
    : d(new Private(-1, -1))
{
}

CollectionQuotaAttribute::CollectionQuotaAttribute(qint64 currentValue, qint64 maxValue)
    : d(new Private(currentValue, maxValue))
{
}

CollectionQuotaAttribute::~CollectionQuotaAttribute()
{
    delete d;
}

void CollectionQuotaAttribute::setCurrentValue(qint64 value)
{
    d->mCurrentValue = value;
}

void CollectionQuotaAttribute::setMaximumValue(qint64 value)
{
    d->mMaximumValue = value;
}

qint64 CollectionQuotaAttribute::currentValue() const
{
    return d->mCurrentValue;
}

qint64 CollectionQuotaAttribute::maximumValue() const
{
    return d->mMaximumValue;
}

QByteArray CollectionQuotaAttribute::type() const
{
    static const QByteArray sType( "collectionquota" );
    return sType;
}

Akonadi::Attribute *CollectionQuotaAttribute::clone() const
{
    return new CollectionQuotaAttribute(d->mCurrentValue, d->mMaximumValue);
}

QByteArray CollectionQuotaAttribute::serialized() const
{
    return QByteArray::number(d->mCurrentValue)
           + ' '
           + QByteArray::number(d->mMaximumValue);
}

void CollectionQuotaAttribute::deserialize(const QByteArray &data)
{
    d->mCurrentValue = -1;
    d->mMaximumValue = -1;

    const QList<QByteArray> items = data.simplified().split(' ');

    if (items.isEmpty()) {
        return;
    }

    d->mCurrentValue = items[0].toLongLong();

    if (items.size() < 2) {
        return;
    }

    d->mMaximumValue = items[1].toLongLong();
}
