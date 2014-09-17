/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "collectionstatistics.h"

#include <QtCore/QSharedData>
#include <QtCore/QDebug>

using namespace Akonadi;

/**
 * @internal
 */
class CollectionStatistics::Private : public QSharedData
{
public:
    Private()
        : QSharedData()
        , count(-1)
        , unreadCount(-1)
        , size(-1)
    {
    }

    Private(const Private &other)
        : QSharedData(other)
    {
        count = other.count;
        unreadCount = other.unreadCount;
        size = other.size;
    }

    qint64 count;
    qint64 unreadCount;
    qint64 size;
};

CollectionStatistics::CollectionStatistics()
    : d(new Private)
{
}

CollectionStatistics::CollectionStatistics(const CollectionStatistics &other)
    : d(other.d)
{
}

CollectionStatistics::~CollectionStatistics()
{
}

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
    return d << "CollectionStatistics:" << endl
           << "   count:" << s.count() << endl
           << "   unread count:" << s.unreadCount() << endl
           << "   size:" << s.size();
}
