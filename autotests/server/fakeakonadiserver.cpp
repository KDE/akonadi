/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "fakeakonadiserver.h"
#include "fakeconnection.h"
#include "fakedatastore.h"
#include "fakesearchmanager.h"
#include "fakeclient.h"
#include "fakeitemretrievalmanager.h"
#include "inspectablenotificationcollector.h"

#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QTest>
#include <QBuffer>
#include <QStandardPaths>

#include <private/protocol_p.h>
#include <private/scope_p.h>
#include <private/standarddirs_p.h>
#include <shared/akapplication.h>

#include "aklocalserver.h"
#include "storage/dbconfig.h"
#include "storage/datastore.h"
#include "preprocessormanager.h"
#include "search/searchmanager.h"
#include "utils.h"

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Akonadi::Server::InspectableNotificationCollector*)

TestScenario TestScenario::create(qint64 tag, TestScenario::Action action,
                                  const Protocol::CommandPtr &response)
{
    TestScenario sc;
    sc.action = action;

    QBuffer buffer(&sc.data);
    buffer.open(QIODevice::ReadWrite);
    {
        QDataStream stream(&buffer);
        stream << tag;
        Protocol::serialize(&buffer, response);
    }

    {
        buffer.seek(0);
        QDataStream os(&buffer);
        qint64 cmpTag;
        os >> cmpTag;
        Q_ASSERT(cmpTag == tag);
        Protocol::CommandPtr cmpResp = Protocol::deserialize(os.device());

        bool ok = false;
        [cmpTag, tag, cmpResp, response, &ok]() {
            QCOMPARE(cmpTag, tag);
            QCOMPARE(cmpResp->type(), response->type());
            QCOMPARE(cmpResp->isResponse(), response->isResponse());
            QCOMPARE(Protocol::debugString(cmpResp), Protocol::debugString(response));
            QCOMPARE(*cmpResp, *response);
            ok = true;
        }();
        if (!ok) {
            sc.data.clear();
            return sc;
        }
    }
    return sc;
}



FakeAkonadiServer *FakeAkonadiServer::instance()
{
    if (!AkonadiServer::s_instance) {
        AkonadiServer::s_instance = new FakeAkonadiServer;
    }

    Q_ASSERT(qobject_cast<FakeAkonadiServer *>(AkonadiServer::s_instance));
    return qobject_cast<FakeAkonadiServer *>(AkonadiServer::s_instance);
}

FakeAkonadiServer::FakeAkonadiServer()
    : AkonadiServer()
    , mDataStore(nullptr)
    , mSearchManager(nullptr)
    , mConnection(nullptr)
    , mClient(nullptr)
    , mRetrievalManager(nullptr)
    , mServerLoop(nullptr)
    , mNtfCollector(nullptr)
    , mPopulateDb(true)
    , mDisableItemRetrievalManager(false)
{
    qputenv("AKONADI_INSTANCE", qPrintable(instanceName()));
    qputenv("XDG_DATA_HOME", qPrintable(QString(basePath() + QLatin1String("/local"))));
    qputenv("XDG_CONFIG_HOME", qPrintable(QString(basePath() + QLatin1String("/config"))));
    qputenv("HOME", qPrintable(basePath()));
    qputenv("KDEHOME", qPrintable(basePath() + QLatin1String("/kdehome")));

    mClient = new FakeClient;

    FakeDataStore::registerFactory();
}

FakeAkonadiServer::~FakeAkonadiServer()
{
    delete mClient;
    delete mConnection;
    delete mRetrievalManager;
    delete mNtfCollector;
}

QString FakeAkonadiServer::basePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                + QStringLiteral("/akonadiserver-test-%1").arg(QCoreApplication::instance()->applicationPid());
}

QString FakeAkonadiServer::socketFile()
{
    return basePath() % QStringLiteral("/local/share/akonadi/akonadiserver.socket");
}

QString FakeAkonadiServer::instanceName()
{
    return QStringLiteral("akonadiserver-test-%1").arg(QCoreApplication::instance()->applicationPid());
}

TestScenario::List FakeAkonadiServer::loginScenario(const QByteArray &sessionId)
{
    SchemaVersion schema = SchemaVersion::retrieveAll().first();

    auto hello = Protocol::HelloResponsePtr::create();
    hello->setServerName(QStringLiteral("Akonadi"));
    hello->setMessage(QStringLiteral("Not Really IMAP server"));
    hello->setProtocolVersion(Protocol::version());
    hello->setGeneration(schema.generation());

    return {
        TestScenario::create(0, TestScenario::ServerCmd, hello),
        TestScenario::create(1,TestScenario::ClientCmd,
                             Protocol::LoginCommandPtr::create(sessionId.isEmpty() ? instanceName().toLatin1() : sessionId)),
        TestScenario::create(1, TestScenario::ServerCmd,
                             Protocol::LoginResponsePtr::create())
    };
}

TestScenario::List FakeAkonadiServer::selectResourceScenario(const QString &name)
{
    const Resource resource = Resource::retrieveByName(name);
    return {
        TestScenario::create(3, TestScenario::ClientCmd,
                             Protocol::SelectResourceCommandPtr::create(resource.name())),
        TestScenario::create(3, TestScenario::ServerCmd,
                             Protocol::SelectResourceResponsePtr::create())
    };
}

void FakeAkonadiServer::disableItemRetrievalManager()
{
    mDisableItemRetrievalManager = true;
}

bool FakeAkonadiServer::init()
{
    qDebug() << "==== Fake Akonadi Server starting up ====";

    qputenv("XDG_DATA_HOME", qPrintable(QString(basePath() + QLatin1String("/local"))));
    qputenv("XDG_CONFIG_HOME", qPrintable(QString(basePath() + QLatin1String("/config"))));
    qputenv("AKONADI_INSTANCE", qPrintable(instanceName()));
    QSettings settings(StandardDirs::serverConfigFile(StandardDirs::WriteOnly), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("General"));
    settings.setValue(QStringLiteral("Driver"), QLatin1String("QSQLITE3"));
    settings.endGroup();

    settings.beginGroup(QStringLiteral("QSQLITE3"));
    settings.setValue(QStringLiteral("Name"), QString(basePath() + QLatin1String("/local/share/akonadi/akonadi.db")));
    settings.endGroup();
    settings.sync();

    DbConfig *dbConfig = DbConfig::configuredDatabase();
    if (dbConfig->driverName() != QLatin1String("QSQLITE3")) {
        throw FakeAkonadiServerException(QLatin1String("Unexpected driver specified. Expected QSQLITE3, got ") + dbConfig->driverName());
    }

    const QLatin1String initCon("initConnection");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(DbConfig::configuredDatabase()->driverName(), initCon);
        DbConfig::configuredDatabase()->apply(db);
        db.setDatabaseName(DbConfig::configuredDatabase()->databaseName());
        if (!db.isDriverAvailable(DbConfig::configuredDatabase()->driverName())) {
            throw FakeAkonadiServerException(QStringLiteral("SQL driver %s not available").arg(db.driverName()));
        }
        if (!db.isValid()) {
            throw FakeAkonadiServerException("Got invalid database");
        }
        if (db.open()) {
            qWarning() << "Database" << dbConfig->configuredDatabase()->databaseName() << "already exists, the test is not running in a clean environment!";
        }
        db.close();
    }

    QSqlDatabase::removeDatabase(initCon);

    dbConfig->setup();

    mDataStore = static_cast<FakeDataStore *>(FakeDataStore::self());
    mDataStore->setPopulateDb(mPopulateDb);
    if (!mDataStore->init()) {
        throw FakeAkonadiServerException("Failed to initialize datastore");
    }

    PreprocessorManager::init();
    PreprocessorManager::instance()->setEnabled(false);
    mSearchManager = new FakeSearchManager();

    if (!mDisableItemRetrievalManager) {
        mRetrievalManager = new FakeItemRetrievalManager();
    }

    const QString socketFile = basePath() + QLatin1String("/local/share/akonadi/akonadiserver.socket");

    qDebug() << "==== Fake Akonadi Server started ====";
    return true;
}

bool FakeAkonadiServer::quit()
{
    qDebug() << "==== Fake Akonadi Server shutting down ====";

    // Stop listening for connections
    if (mCmdServer) {
        mCmdServer->close();
    }

    if (!qEnvironmentVariableIsSet("AKONADI_TEST_NOCLEANUP")) {
        bool ok = QDir(basePath()).removeRecursively();
        qDebug() << "Cleaned up" << basePath() << "success=" << ok;
    } else {
        qDebug() << "Skipping clean up of" << basePath();
    }

    PreprocessorManager::done();
    (void)SearchManager::instance();

    if (mDataStore) {
        mDataStore->close();
    }

    qDebug() << "==== Fake Akonadi Server shut down ====";
    return true;
}

void FakeAkonadiServer::setScenarios(const TestScenario::List &scenarios)
{
    mClient->setScenarios(scenarios);
}

void FakeAkonadiServer::newCmdConnection(quintptr socketDescriptor)
{
    mConnection = new FakeConnection(socketDescriptor);

    // Connection is it's own thread, so we have to make sure we get collector
    // from DataStore of the Connection's thread, not ours
    NotificationCollector *collector = nullptr;
    QMetaObject::invokeMethod(mConnection, "notificationCollector", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(Akonadi::Server::NotificationCollector*, collector));
    mNtfCollector = dynamic_cast<InspectableNotificationCollector*>(collector);
    Q_ASSERT(mNtfCollector);
    mNotificationSpy.reset(new QSignalSpy(mNtfCollector, &Server::InspectableNotificationCollector::notifySignal));
    Q_ASSERT(mNotificationSpy->isValid());
}

void FakeAkonadiServer::runTest()
{
    mCmdServer = new AkLocalServer(this);
    connect(mCmdServer, static_cast<void(AkLocalServer::*)(quintptr)>(&AkLocalServer::newConnection),
            this, &FakeAkonadiServer::newCmdConnection);
    QVERIFY(mCmdServer->listen(socketFile()));

    mServerLoop = new QEventLoop(this);
    connect(mClient, &QThread::finished, mServerLoop, &QEventLoop::quit);

    // Start the client: the client will connect to the server and will
    // start playing the scenario
    mClient->start();

    // Wait until the client disconnects, i.e. until the scenario is completed.
    mServerLoop->exec();

    mServerLoop->deleteLater();
    mServerLoop = nullptr;

    mCmdServer->close();

    // Flush any pending notifications
    mNtfCollector->dispatchNotifications();
}

QSharedPointer<QSignalSpy> FakeAkonadiServer::notificationSpy() const
{
    return mNotificationSpy;
}

void FakeAkonadiServer::setPopulateDb(bool populate)
{
    mPopulateDb = populate;
}
