/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "fakeconnection.h"

#include "fakeakonadiserver.h"
#include "fakedatastore.h"

using namespace Akonadi::Server;

FakeConnection::FakeConnection(quintptr socketDescriptor, FakeAkonadiServer &akonadi)
    : Connection(socketDescriptor, akonadi)
{
}

FakeConnection::FakeConnection(AkonadiServer &akonadi)
    : Connection(akonadi)
{
}

FakeConnection::~FakeConnection()
{
    quitThread();
}

void FakeConnection::init()
{
    Connection::init();

    mNotificationCollector = storageBackend()->notificationCollector();
}

void FakeConnection::quit()
{
    mNotificationCollector->dispatchNotifications();
    Connection::quit();
}

NotificationCollector *FakeConnection::notificationCollector()
{
    return mNotificationCollector;
}
