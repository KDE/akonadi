/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "cachepolicy.h"
#include "collection.h"

using namespace Akonadi;

/**
 * @internal
 */
class Akonadi::CachePolicyPrivate : public QSharedData
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
    static QSharedDataPointer<CachePolicyPrivate> sharedPrivate(new CachePolicyPrivate);
    d = sharedPrivate;
}

CachePolicy::CachePolicy(const CachePolicy &other)
    : d(other.d)
{
}

CachePolicy::~CachePolicy()
{
}

CachePolicy &CachePolicy::operator=(const CachePolicy &other)
{
    d = other.d;
    return *this;
}

bool Akonadi::CachePolicy::operator==(const CachePolicy &other) const
{
    if (!d->inherit && !other.d->inherit) {
        return d->localParts == other.d->localParts && d->timeout == other.d->timeout && d->interval == other.d->interval
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
