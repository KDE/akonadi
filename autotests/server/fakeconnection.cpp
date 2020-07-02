/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "fakeconnection.h"

#include "fakedatastore.h"
#include "fakeakonadiserver.h"


using namespace Akonadi::Server;

FakeConnection::FakeConnection(quintptr socketDescriptor, FakeAkonadiServer &akonadi)
    : Connection(socketDescriptor, akonadi)
{
}

FakeConnection::FakeConnection(AkonadiServer &akonadi)
    : Connection(akonadi)
{
}

FakeConnection::~FakeConnection() = default;

NotificationCollector *FakeConnection::notificationCollector()
{
    return storageBackend()->notificationCollector();
}
