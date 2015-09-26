/*
    Copyright (c) 2015 Daniel Vrátil <dvratil@redhat.com>

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

#include "notificationbus_p.h"
#include "session_p.h"

#include <akonadi/private/protocol_p.h>

#include <QTimer>

using namespace Akonadi;

NotificationBusPrivate::NotificationBusPrivate(Session *parent)
    : QObject(parent)
    , SessionPrivate(parent)
{
}

NotificationBusPrivate::~NotificationBusPrivate()
{
}

bool NotificationBusPrivate::handleCommand(qint64 tag, const Protocol::Command &cmd)
{
    Q_UNUSED(tag);

    if (cmd.type() == Protocol::Command::Hello) {
        Protocol::HelloResponse hello(cmd);
        if (hello.isError()) {
            qWarning() << "Error when establishing connection with Akonadi server:" << hello.errorMessage();
            socket->close();
            QTimer::singleShot(1000, mParent, SLOT(reconnect()));
            return false;
        }

        qDebug() << "Connected to" << hello.serverName() << ", using protocol version" << hello.protocolVersion();
        qDebug() << "Server says:" << hello.message();
        // Version mismatch is handled in SessionPrivate::startJob() so that
        // we can report the error out via KJob API
        protocolVersion = hello.protocolVersion();

        Protocol::LoginCommand login(sessionId, Protocol::LoginCommand::NotificationBus);
        sendCommand(nextTag(), login);
        return true;
    }

    if (cmd.type() == Protocol::Command::Login) {
        Protocol::LoginResponse login(cmd);
        if (login.isError()) {
            qWarning() << "Unable to login to Akonadi server:" << login.errorMessage();
            socket->close();
            QTimer::singleShot(1000, mParent, SLOT(reconnect()));
            return false;
        }

        connected = true;
        startNext();
        return true;
    }

    if (cmd.type() == Protocol::Command::ChangeNotification) {
        Q_EMIT notify(cmd);
        return true;
    }

    qWarning() << "Recieved invalid command on NotificationBus" << sessionId;
    return false;
}
