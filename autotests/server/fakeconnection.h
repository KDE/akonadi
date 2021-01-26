/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_FAKECONNECTION
#define AKONADI_FAKECONNECTION

#include "connection.h"

namespace Akonadi
{
namespace Server
{
class NotificationCollector;
class FakeAkonadiServer;

class FakeConnection : public Connection
{
    Q_OBJECT

public:
    explicit FakeConnection(quintptr socketDescriptor, FakeAkonadiServer &akonadi);
    explicit FakeConnection(AkonadiServer &akonadi);
    ~FakeConnection() override;

public Q_SLOTS:
    Akonadi::Server::NotificationCollector *notificationCollector();
};

}
}

#endif
