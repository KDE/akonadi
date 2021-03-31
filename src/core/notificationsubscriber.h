/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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

    ItemFetchScope itemFetchScope() const;
    void setItemFetchScope(const ItemFetchScope &itemFetchScope);

    CollectionFetchScope collectionFetchScope() const;
    void setCollectionFetchScope(const CollectionFetchScope &collectionFetchScope);

    TagFetchScope tagFetchScope() const;
    void setTagFetchScope(const TagFetchScope &tagFetchScope);

private:
    class Private;
    QSharedDataPointer<Private> d;
};

}
