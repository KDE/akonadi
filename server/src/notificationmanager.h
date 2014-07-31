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

#include "../libs/notificationmessage_p.h"
#include "../libs/notificationmessagev3_p.h"
#include "storage/entity.h"

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtDBus/qdbuscontext.h>

class NotificationManagerTest;

namespace Akonadi {
namespace Server {

class NotificationCollector;
class NotificationSource;

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

public Q_SLOTS:
    Q_SCRIPTABLE void emitPendingNotifications();

    /**
     * Subscribe to notifications emitted by this manager.
     *
     * @param identifier Identifier to use of our subscription.
     * @return The path we got assigned. Contains identifier.
     */
    Q_SCRIPTABLE QDBusObjectPath subscribe(const QString &identifier);

    /**
     * Subscribe to notifications emitted by this manager.
     *
     * @param identifier Identifier to use for our subscription.
     * @param serverSideMonitor Whether client supports server-side monitoring
     * @return The path we got assigned. Contains identifier.
     */
    Q_SCRIPTABLE QDBusObjectPath subscribeV2(const QString &identifier, bool serverSideMonitor);

    /**
     * Subscribe to notifications emitted by this manager.
     *
     * @param identifier Identifier to use for our subscription.
     * @param serverSideMonitor Whether client supports server-side monitoring
     * @param exclusive Exclusive subscribers also receive notifications on referenced collections
     * @return The path we got assigned. Contains identifier.
     */
    Q_SCRIPTABLE QDBusObjectPath subscribeV3(const QString &identifier, bool serverSideMonitor, bool exclusive);

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
    Q_SCRIPTABLE QStringList subscribers() const;

Q_SIGNALS:
    Q_SCRIPTABLE void notify(const Akonadi::NotificationMessage::List &msgs);

    Q_SCRIPTABLE void subscribed(const QString &identifier);
    Q_SCRIPTABLE void unsubscribed(const QString &identifier);

private Q_SLOTS:
    void slotNotify(const Akonadi::NotificationMessageV3::List &msgs);

private:
    NotificationManager();

private:
    void registerSource(NotificationSource *source);
    void unregisterSource(NotificationSource *source);

    static NotificationManager *mSelf;
    NotificationMessageV3::List mNotifications;
    QTimer mTimer;

    //! One message source for each subscribed process
    QHash<QString, NotificationSource *> mNotificationSources;

    friend class NotificationSource;
    friend class ::NotificationManagerTest;
};

} // namespace Server
} // namespace Akonadi

#endif
