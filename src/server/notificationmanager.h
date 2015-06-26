/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_NOTIFICATIONMANAGER_H
#define AKONADI_NOTIFICATIONMANAGER_H

#include <private/protocol_p.h>
#include "storage/entity.h"

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QMutex>
#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusObjectPath>

class NotificationManagerTest;

namespace Akonadi {
namespace Server {

class NotificationCollector;
class NotificationSource;
class Connection;

/**
  Notification manager D-Bus interface.
*/
class NotificationManager : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Akonadi.NotificationManager")

public:
    static NotificationManager *self();

    virtual ~NotificationManager();

    void connectNotificationCollector(NotificationCollector *collector);

    void registerConnection(Connection *connection);
    void unregisterConnection(Connection *connection);

public Q_SLOTS:
    Q_SCRIPTABLE void emitPendingNotifications();

    /**
     * Subscribe to notifications emitted by this manager.
     *
     * @param identifier Identifier to use for our subscription.
     * @param exclusive Exclusive subscribers also receive notifications on referenced collections
     * @return The path we got assigned. Contains identifier.
     */
    Q_SCRIPTABLE QDBusObjectPath subscribe(const QString &identifier, bool exclusive);

    /**
     * Unsubscribe from this manager.
     *
     * This method is for your inconvenience only. It's advisable to use the unsubscribe method
     * provided by the NotificationSource.
     *
     * @param identifier The identifier used for subscription.
     */
    Q_SCRIPTABLE void unsubscribe(const QString &identifier);

    /**
     * Returns identifiers of currently subscribed sources
     */
    Q_SCRIPTABLE QList<QDBusObjectPath> subscribers() const;

Q_SIGNALS:
    Q_SCRIPTABLE void notify(const Akonadi::Protocol::ChangeNotification::List &msgs);

    Q_SCRIPTABLE void subscribed(const QDBusObjectPath &path);
    Q_SCRIPTABLE void unsubscribed(const QDBusObjectPath &path);

private Q_SLOTS:
    void slotNotify(const Akonadi::Protocol::ChangeNotification::List &msgs);

private:
    NotificationManager();

private:
    void registerSource(NotificationSource *source);
    void unregisterSource(NotificationSource *source);

    static NotificationManager *mSelf;
    Protocol::ChangeNotification::List mNotifications;
    QTimer mTimer;

    //! One message source for each subscribed process
    QMutex mSourcesLock;
    QHash<QString, NotificationSource *> mNotificationSources;

    friend class NotificationSource;
    friend class ::NotificationManagerTest;
};

} // namespace Server
} // namespace Akonadi

#endif
