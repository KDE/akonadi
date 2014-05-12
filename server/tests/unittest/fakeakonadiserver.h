/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
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

#ifndef FAKEAKONADISERVER_H
#define FAKEAKONADISERVER_H

#include <QByteArray>

// FakeAkonadiServer is included from all Handler tests, so include all other fake
// classes from here to make it easier to use
#include "fakedatastore.h"
#include "fakeconnection.h"

namespace Akonadi {
namespace Server {

class FakeAkonadiServer
{
public:
    explicit FakeAkonadiServer();
    ~FakeAkonadiServer();

    FakeDataStore *dataStore();
    FakeConnection *connection(Handler *handler, const QByteArray &command,
                               const CommandContext &context = CommandContext());

    bool start();

private:
    QString basePath() const;
    QString instanceName() const;

    bool deleteDirectory(const QString &path);

    bool cleanup();
    bool abortSetup(const QString &msg);


    FakeConnection *mConnection;
    FakeDataStore *mDataStore;
};

}
}

#endif // FAKEAKONADISERVER_H
