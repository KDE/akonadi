/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>                 *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#pragma once

#include "akonadiconnection.h"
#include "teststoragebackend.h"

using namespace Akonadi;

static AkonadiConnection *s_connection = nullptr;
static DataStore *s_backend = nullptr;

class MockConnection : public AkonadiConnection
{
public:
    MockConnection()
    {
    }
    DataStore *storageBackend()
    {
        if (!s_backend) {
            s_backend = new MockBackend();
        }
        return s_backend;
    }
};

class MockObjects
{
public:
    MockObjects();
    ~MockObjects();

    static AkonadiConnection *mockConnection()
    {
        if (!s_connection) {
            s_connection = new MockConnection();
        }
        return s_connection;
    }
}; // End of class MockObjects

