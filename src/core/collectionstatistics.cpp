/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionstatistics.h"

#include <QSharedData>
#include <QDebug>

using namespace Akonadi;

/**
 * @internal
 */
class Q_DECL_HIDDEN CollectionStatistics::Private : public QSharedData
{
public:
    qint64 count = -1;
    qint64 unreadCount = -1;
    qint64 size = -1;
};

CollectionStatistics::CollectionStatistics()
    : d(new Private)
{
}

CollectionStatistics::CollectionStatistics(const CollectionStatistics &other)
    : d(other.d)
{
}

CollectionStatistics::~CollectionStatistics() = default;

qint64 CollectionStatistics::count() const
{
    return d->count;
}

void CollectionStatistics::setCount(qint64 count)
{
    d->count = count;
}

qint64 CollectionStatistics::unreadCount() const
{
    return d->unreadCount;
}

void CollectionStatistics::setUnreadCount(qint64 count)
{
    d->unreadCount = count;
}

qint64 CollectionStatistics::size() const
{
    return d->size;
}

void CollectionStatistics::setSize(qint64 size)
{
    d->size = size;
}

CollectionStatistics &CollectionStatistics::operator =(const CollectionStatistics &other)
{
    d = other.d;
    return *this;
}

QDebug operator<<(QDebug d, const CollectionStatistics &s)
{
    return d << "CollectionStatistics:\n"
             << "      count:" << s.count() << '\n'
             << "      unread count:" << s.unreadCount() << '\n'
             << "      size:" << s.size();
}
