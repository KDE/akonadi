/*
    Copyright (c) 2011 Grégory Oestreicher <greg@kamago.net>

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

#include "freebusyproviderbase.h"
#include "freebusyproviderbase_p.h"

#include "../dbusconnectionpool.h"
#include "freebusyprovideradaptor.h"

#include <kdatetime.h>

using namespace Akonadi;

FreeBusyProviderBasePrivate::FreeBusyProviderBasePrivate(FreeBusyProviderBase *qq)
    : QObject(), q(qq)
{
    new Akonadi__FreeBusyProviderAdaptor(this);
    DBusConnectionPool::threadConnection().registerObject(QStringLiteral("/FreeBusyProvider"),
            this, QDBusConnection::ExportAdaptors);
}

QString FreeBusyProviderBasePrivate::lastCacheUpdate()
{
    KDateTime last = q->lastCacheUpdate();
    return last.toString();
}

void FreeBusyProviderBasePrivate::canHandleFreeBusy(const QString &email)
{
    q->canHandleFreeBusy(email);
}

void FreeBusyProviderBasePrivate::retrieveFreeBusy(const QString &email, const QString &_start, const QString &_end)
{
    KDateTime start = KDateTime::fromString(_start);
    KDateTime end = KDateTime::fromString(_end);
    q->retrieveFreeBusy(email, start, end);
}

FreeBusyProviderBase::FreeBusyProviderBase()
    : d(new FreeBusyProviderBasePrivate(this))
{
}

FreeBusyProviderBase::~FreeBusyProviderBase()
{
    delete d;
}

void FreeBusyProviderBase::handlesFreeBusy(const QString &email, bool handles) const
{
    emit d->handlesFreeBusy(email, handles);
}

void FreeBusyProviderBase::freeBusyRetrieved(const QString &email, const QString &freeBusy, bool success, const QString &errorText)
{
    emit d->freeBusyRetrieved(email, freeBusy, success, errorText);
}

#include "moc_freebusyproviderbase_p.cpp"
