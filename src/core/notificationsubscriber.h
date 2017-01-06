/*
    Copyright (c) 2016 Daniel Vrátil <dvratil@kde.org>

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

#ifndef AKONADI_NOTIFICATIONSUBSCRIBER_H
#define AKONADI_NOTIFICATIONSUBSCRIBER_H

#include <akonadicore_export.h>

#include "monitor.h"
#include <QSet>

namespace Akonadi
{

class AKONADICORE_EXPORT NotificationSubscriber
{
public:
    explicit NotificationSubscriber();
    NotificationSubscriber(const NotificationSubscriber &other);
    ~NotificationSubscriber();

    NotificationSubscriber &operator=(const NotificationSubscriber &other);

    bool isValid() const;

    QByteArray subscriber() const;
    void setSubscriber(const QByteArray &subscriber);

    QByteArray sessionId() const;
    void setSessionId(const QByteArray &sessionId);

    QSet<qint64> monitoredCollections() const;
    void setMonitoredCollections(const QSet<qint64> &collections);

    QSet<qint64> monitoredItems() const;
    void setMonitoredItems(const QSet<qint64> &items);

    QSet<qint64> monitoredTags() const;
    void setMonitoredTags(const QSet<qint64> &tags);

    QSet<Monitor::Type> monitoredTypes() const;
    void setMonitoredTypes(const QSet<Monitor::Type> &type);

    QSet<QString> monitoredMimeTypes() const;
    void setMonitoredMimeTypes(const QSet<QString> &mimeTypes);

    QSet<QByteArray> monitoredResources() const;
    void setMonitoredResources(const QSet<QByteArray> &resources);

    QSet<QByteArray> ignoredSessions() const;
    void setIgnoredSessions(const QSet<QByteArray> &ignoredSessions);

    bool isAllMonitored() const;
    void setIsAllMonitored(bool isAllMonitored);

    bool isExclusive() const;
    void setIsExclusive(bool isExclusive);

private:
    class Private;
    QSharedDataPointer<Private> d;
};

}
#endif
