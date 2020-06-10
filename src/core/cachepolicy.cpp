/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "cachepolicy.h"
#include "collection.h"

using namespace Akonadi;

/**
 * @internal
 */
class Q_DECL_HIDDEN CachePolicy::Private : public QSharedData
{
public:
    QStringList localParts;
    int timeout = -1;
    int interval = -1;
    bool inherit = true;
    bool syncOnDemand = false;
};

CachePolicy::CachePolicy()
{
    static QSharedDataPointer<Private> sharedPrivate(new Private);
    d = sharedPrivate;
}

CachePolicy::CachePolicy(const CachePolicy &other)
    : d(other.d)
{
}

CachePolicy::~CachePolicy()
{
}

CachePolicy &CachePolicy::operator =(const CachePolicy &other)
{
    d = other.d;
    return *this;
}

bool Akonadi::CachePolicy::operator ==(const CachePolicy &other) const
{
    if (!d->inherit && !other.d->inherit) {
        return d->localParts == other.d->localParts
               && d->timeout == other.d->timeout
               && d->interval == other.d->interval
               && d->syncOnDemand == other.d->syncOnDemand;
    }
    return d->inherit == other.d->inherit;
}

bool CachePolicy::inheritFromParent() const
{
    return d->inherit;
}

void CachePolicy::setInheritFromParent(bool inherit)
{
    d->inherit = inherit;
}

QStringList CachePolicy::localParts() const
{
    return d->localParts;
}

void CachePolicy::setLocalParts(const QStringList &parts)
{
    d->localParts = parts;
}

int CachePolicy::cacheTimeout() const
{
    return d->timeout;
}

void CachePolicy::setCacheTimeout(int timeout)
{
    d->timeout = timeout;
}

int CachePolicy::intervalCheckTime() const
{
    return d->interval;
}

void CachePolicy::setIntervalCheckTime(int time)
{
    d->interval = time;
}

bool CachePolicy::syncOnDemand() const
{
    return d->syncOnDemand;
}

void CachePolicy::setSyncOnDemand(bool enable)
{
    d->syncOnDemand = enable;
}

QDebug operator<<(QDebug d, const CachePolicy &c)
{
    return d << "CachePolicy: \n"
           << "   inherit:" << c.inheritFromParent() << '\n'
           << "   interval:" << c.intervalCheckTime() << '\n'
           << "   timeout:" << c.cacheTimeout() << '\n'
           << "   sync on demand:" << c.syncOnDemand() << '\n'
           << "   local parts:" << c.localParts();
}
