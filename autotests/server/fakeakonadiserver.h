/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#pragma once

#include "akonadi.h"
#include "exception.h"

#include <QSharedPointer>
#include <QSignalSpy>

#include <memory>
#include <type_traits>

#include <private/protocol_p.h>

namespace Akonadi
{
namespace Server
{
class InspectableNotificationCollector;
class FakeDataStore;
class FakeConnection;
class FakeClient;

class TestScenario
{
public:
    typedef QList<TestScenario> List;

    enum Action {
        ServerCmd,
        ClientCmd,
        Wait,
        Quit,
        Ignore,
    };

    Action action;
    QByteArray data;

    static TestScenario create(qint64 tag, Action action, const Protocol::CommandPtr &response);

    static TestScenario wait(int timeout)
    {
        return TestScenario{Wait, QByteArray::number(timeout)};
    }

    static TestScenario quit()
    {
        return TestScenario{Quit, QByteArray()};
    }

    static TestScenario ignore(int count)
    {
        return TestScenario{Ignore, QByteArray::number(count)};
    }
};

/**
 * A fake server used for testing. Loosely based on KIMAP::FakeServer
 */
class FakeAkonadiServer : public AkonadiServer
{
    Q_OBJECT

public:
    explicit FakeAkonadiServer();
    ~FakeAkonadiServer() override;

    /* Reimpl */
    bool init() override;

    static QString basePath();
    static QString socketFile();
    static QString instanceName();

    static TestScenario::List loginScenario(const QByteArray &sessionId = QByteArray());
    static TestScenario::List selectCollectionScenario(const QString &name);
    static TestScenario::List selectResourceScenario(const QString &name);

    void setScenarios(const TestScenario::List &scenarios);

    void runTest();

    QSharedPointer<QSignalSpy> notificationSpy() const;

    void setPopulateDb(bool populate);
    void disableItemRetrievalManager();

protected:
    void newCmdConnection(quintptr socketDescriptor) override;

private:
    bool quit() override;
    void initFake();

    FakeDataStore *mDataStore = nullptr;
    std::unique_ptr<FakeConnection> mConnection;
    std::unique_ptr<FakeClient> mClient;

    InspectableNotificationCollector *mNtfCollector = nullptr;
    QSharedPointer<QSignalSpy> mNotificationSpy;

    bool mPopulateDb = true;
    bool mDisableItemRetrievalManager = false;
};

AKONADI_EXCEPTION_MAKE_INSTANCE(FakeAkonadiServerException);

}
}

Q_DECLARE_METATYPE(Akonadi::Server::TestScenario)
Q_DECLARE_METATYPE(Akonadi::Server::TestScenario::List)

