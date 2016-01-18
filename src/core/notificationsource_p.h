/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef NOTIFICATIONSOURCE_P_H
#define NOTIFICATIONSOURCE_P_H

#include <QtCore/QObject>

#include "akonadiprivate_export.h"

#include "tag.h"
#include "collection.h"
#include "item.h"

#include "private/protocol_p.h"

namespace Akonadi
{

class AKONADI_TESTS_EXPORT NotificationSource : public QObject
{
    Q_OBJECT

public:
    NotificationSource(QObject *source);
    ~NotificationSource();

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

#endif // NOTIFICATIONSOURCE_P_H
