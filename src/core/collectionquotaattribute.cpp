/*
    SPDX-FileCopyrightText: 2009 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionquotaattribute.h"

#include <QByteArray>

using namespace Akonadi;

class Q_DECL_HIDDEN CollectionQuotaAttribute::Private
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
    static const QByteArray sType("collectionquota");
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
