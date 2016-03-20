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

#include "akthread.h"

#include <private/protocol_p.h>

#include <QtCore/QTimer>

class NotificationManagerTest;
class QLocalSocket;

namespace Akonadi {
namespace Server {

class NotificationCollector;
class NotificationSubscriber;

class NotificationManager : public AkThread
{
    Q_OBJECT

public:
    explicit NotificationManager();
    virtual ~NotificationManager();

    void connectNotificationCollector(NotificationCollector *collector);

public Q_SLOTS:
    void registerConnection(quintptr socketDescriptor);

    void emitPendingNotifications();

private Q_SLOTS:
    void slotNotify(const Akonadi::Protocol::ChangeNotification::List &msgs);

protected:
    void init() Q_DECL_OVERRIDE;
    void quit() Q_DECL_OVERRIDE;

private:
    Protocol::ChangeNotification::List mNotifications;
    QTimer *mTimer;

    QVector<NotificationSubscriber *> mSubscribers;

    friend class NotificationSubscriber;
    friend class ::NotificationManagerTest;
};

} // namespace Server
} // namespace Akonadi

#endif
