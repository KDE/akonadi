/*
    SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

#include "akonaditests_export.h"

#include "collection.h"
#include "item.h"
#include "tag.h"

#include "private/protocol_p.h"

namespace Akonadi
{
class AKONADI_TESTS_EXPORT NotificationSource : public QObject
{
    Q_OBJECT

public:
    explicit NotificationSource(QObject *source);
    ~NotificationSource() override;

    QString identifier() const;

    void setAllMonitored(bool allMonitored);
    void setExclusive(bool exclusive);
    void setMonitoredCollection(Collection::Id id, bool monitored);
    void setMonitoredItem(Item::Id id, bool monitored);
    void setMonitoredResource(const QByteArray &resource, bool monitored);
    void setMonitoredMimeType(const QString &mimeType, bool monitored);
    void setMonitoredTag(Tag::Id id, bool monitored);
    void setMonitoredType(Protocol::ChangeNotification::Type type, bool monitored);
    void setIgnoredSession(const QByteArray &session, bool monitored);
    void setSession(const QByteArray &session);

    QObject *source() const;
};

}

