/*
 * Copyright (C) 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef NOTIFICATIONSOURCE_P_H
#define NOTIFICATIONSOURCE_P_H

#include <QtCore/QObject>

#include "entity.h"
#include "akonadiprivate_export.h"
#include <notificationmessagev2_p.h>

namespace Akonadi
{

class AKONADI_TESTS_EXPORT NotificationSource: public QObject
{
    Q_OBJECT

  public:
    NotificationSource( QObject *source );
    ~NotificationSource();

    void setAllMonitored( bool allMonitored );
    void setMonitoredCollection( Entity::Id id, bool monitored );
    void setMonitoredItem( Entity::Id id, bool monitored );
    void setMonitoredResource( const QByteArray &resource, bool monitored );
    void setMonitoredMimeType( const QString &mimeType, bool monitored );
    void setIgnoredSession( const QByteArray &session, bool monitored );

    QObject* source() const;

  Q_SIGNALS:
    void notifyV2( const Akonadi::NotificationMessageV2::List &msgs );
};

}

#endif // NOTIFICATIONSOURCE_P_H
