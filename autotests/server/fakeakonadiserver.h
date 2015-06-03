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

#include "akonadi.h"
#include "exception.h"

#include <QSignalSpy>
#include <QBuffer>
#include <QDataStream>

#include <type_traits>

#include <private/protocol_p.h>

class QLocalServer;
class QEventLoop;

namespace Akonadi {
namespace Server {

class FakeSearchManager;
class FakeDataStore;
class FakeConnection;
class FakeClient;

class TestScenario {
public:
    typedef QList<TestScenario> List;

    enum Action {
        ServerCmd,
        ClientCmd,
        Wait,
        Quit
    };

    Action action;
    QByteArray data;

    template<typename T>
    static typename std::enable_if<std::is_base_of<Protocol::Response, T>::value, TestScenario>::type
    create(qint64 tag, Action action, const T &response) {
        TestScenario sc;
        sc.action = action;
        QDataStream stream(&sc.data, QIODevice::WriteOnly);
        stream << tag
               << response;

        QDataStream os(sc.data);
        qint64 vTag;
        os >> vTag;
        Protocol::Command cmd = Protocol::Factory::fromStream(os);

        Q_ASSERT(vTag == tag);
        Q_ASSERT(cmd.type() == response.type());
        Q_ASSERT(cmd.isResponse() == response.isResponse());
        Q_ASSERT(cmd == response);
        return sc;
    }

    template<typename T>
    static typename std::enable_if<std::is_base_of<Protocol::Response, T>::value == false, TestScenario>::type
    create(qint64 tag, Action action, const T &command, int */* dummy */ = 0)
    {
        TestScenario sc;
        sc.action = action;
        QDataStream stream(&sc.data, QIODevice::WriteOnly);
        stream << tag
               << command;
        return sc;
    }

    static TestScenario wait(int timeout)
    {
        return TestScenario { Wait, QByteArray::number(timeout) };
    }

    static TestScenario quit()
    {
        return TestScenario { Quit, QByteArray() };
    }
};

/**
 * A fake server used for testing. Losely based on KIMAP::FakeServer
 */
class FakeAkonadiServer : public AkonadiServer
{
    Q_OBJECT

public:
    static FakeAkonadiServer *instance();

    ~FakeAkonadiServer();

    /* Reimpl */
    bool init();

    /* Reimpl */
    bool quit();

    FakeDataStore *dataStore() const;

    static QString basePath();
    static QString socketFile();
    static QString instanceName();

    static TestScenario::List loginScenario();
    static TestScenario::List selectCollectionScenario(const QString &name);
    static TestScenario::List selectResourceScenario(const QString &name);

    void setScenarios(const TestScenario::List &scenarios);

    void runTest();

    QSignalSpy *notificationSpy() const;

    void setPopulateDb(bool populate);

protected:
    /* Reimpl */
    void incomingConnection(quintptr socketDescriptor);

private:
    explicit FakeAkonadiServer();

    bool deleteDirectory(const QString &path);

    FakeDataStore *mDataStore;
    FakeSearchManager *mSearchManager;
    FakeConnection *mConnection;
    FakeClient *mClient;

    QEventLoop *mServerLoop;

    QSignalSpy *mNotificationSpy;

    bool mPopulateDb;

    static FakeAkonadiServer *sInstance;
};

AKONADI_EXCEPTION_MAKE_INSTANCE(FakeAkonadiServerException);

}
}

Q_DECLARE_METATYPE(Akonadi::Server::TestScenario)
Q_DECLARE_METATYPE(Akonadi::Server::TestScenario::List)

#endif // FAKEAKONADISERVER_H
