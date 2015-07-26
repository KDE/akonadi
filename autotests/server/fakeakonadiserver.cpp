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

#include "fakeakonadiserver.h"
#include "fakeconnection.h"
#include "fakedatastore.h"
#include "fakesearchmanager.h"
#include "fakeclient.h"

#include <QSettings>
#include <QCoreApplication>
#include <QSqlQuery>
#include <QDir>
#include <QFileInfo>
#include <QLocalServer>
#include <QTest>

#include <private/xdgbasedirs_p.h>
#include <private/protocol_p.h>
#include <private/scope_p.h>
#include <shared/akstandarddirs.h>
#include <shared/akapplication.h>

#include "storage/dbconfig.h"
#include "storage/datastore.h"
#include "preprocessormanager.h"
#include "search/searchmanager.h"
#include "utils.h"

using namespace Akonadi;
using namespace Akonadi::Server;


TestScenario TestScenario::create(qint64 tag, TestScenario::Action action,
                                  const Protocol::Command &response)
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
        Protocol::Command cmpResp = Protocol::deserialize(os.device());

        bool ok = false;
        [cmpTag, tag, cmpResp, response, &ok]() {
            QCOMPARE(cmpTag, tag);
            QCOMPARE(cmpResp.type(), response.type());
            QCOMPARE(cmpResp.isResponse(), response.isResponse());
            QCOMPARE(cmpResp.debugString(), response.debugString());
            QCOMPARE(cmpResp, response);
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
    , mDataStore(0)
    , mServerLoop(0)
    , mNotificationSpy(0)
    , mPopulateDb(true)
{
    qputenv("AKONADI_INSTANCE", qPrintable(instanceName()));
    qputenv("XDG_DATA_HOME", qPrintable(QString(basePath() + QLatin1String("/local"))));
    qputenv("XDG_CONFIG_HOME", qPrintable(QString(basePath() + QLatin1String("/config"))));
    qputenv("HOME", qPrintable(basePath()));
    qputenv("KDEHOME", qPrintable(basePath() + QLatin1String("/kdehome")));

    mClient = new FakeClient;
}

FakeAkonadiServer::~FakeAkonadiServer()
{
    delete mClient;
}

QString FakeAkonadiServer::basePath()
{
    return QString::fromLatin1("/tmp/akonadiserver-test-%1").arg(QCoreApplication::instance()->applicationPid());
}

QString FakeAkonadiServer::socketFile()
{
    return basePath() % QLatin1String("/local/share/akonadi/akonadiserver.socket");
}

QString FakeAkonadiServer::instanceName()
{
    return QString::fromLatin1("akonadiserver-test-%1").arg(QCoreApplication::instance()->applicationPid());
}

TestScenario::List FakeAkonadiServer::loginScenario(const QByteArray &sessionId)
{
    return {
        TestScenario::create(0, TestScenario::ServerCmd,
                             Protocol::HelloResponse(QStringLiteral("Akonadi"),
                                                     QStringLiteral("Not Really IMAP server"),
                                                     Protocol::version())),
        TestScenario::create(1,TestScenario::ClientCmd,
                             Protocol::LoginCommand(sessionId.isEmpty() ? instanceName().toLatin1() : sessionId)),
        TestScenario::create(1, TestScenario::ServerCmd,
                             Protocol::LoginResponse())
    };
}

TestScenario::List FakeAkonadiServer::selectResourceScenario(const QString &name)
{
    const Resource resource = Resource::retrieveByName(name);
    return {
        TestScenario::create(3, TestScenario::ClientCmd,
                             Protocol::SelectResourceCommand(resource.name())),
        TestScenario::create(3, TestScenario::ServerCmd,
                             Protocol::SelectResourceResponse())
    };
}

bool FakeAkonadiServer::init()
{
    qDebug() << "==== Fake Akonadi Server starting up ====";

    qputenv("XDG_DATA_HOME", qPrintable(QString(basePath() + QLatin1String("/local"))));
    qputenv("XDG_CONFIG_HOME", qPrintable(QString(basePath() + QLatin1String("/config"))));
    qputenv("AKONADI_INSTANCE", qPrintable(instanceName()));
    QSettings settings(AkStandardDirs::serverConfigFile(XdgBaseDirs::WriteOnly), QSettings::IniFormat);
    settings.beginGroup(QLatin1String("General"));
    settings.setValue(QLatin1String("Driver"), QLatin1String("QSQLITE3"));
    settings.endGroup();

    settings.beginGroup(QLatin1String("QSQLITE3"));
    settings.setValue(QLatin1String("Name"), QString(basePath() + QLatin1String("/local/share/akonadi/akonadi.db")));
    settings.endGroup();
    settings.sync();

    DbConfig *dbConfig = DbConfig::configuredDatabase();
    if (dbConfig->driverName() != QLatin1String("QSQLITE3")) {
        throw FakeAkonadiServerException(QLatin1String("Unexpected driver specified. Expected QSQLITE3, got ") + dbConfig->driverName());
    }

    const QLatin1String initCon("initConnection");
    QSqlDatabase db = QSqlDatabase::addDatabase(DbConfig::configuredDatabase()->driverName(), initCon);
    DbConfig::configuredDatabase()->apply(db);
    db.setDatabaseName(DbConfig::configuredDatabase()->databaseName());
    if (!db.isDriverAvailable(DbConfig::configuredDatabase()->driverName())) {
        throw FakeAkonadiServerException(QString::fromLatin1("SQL driver %s not available").arg(db.driverName()));
    }
    if (!db.isValid()) {
        throw FakeAkonadiServerException("Got invalid database");
    }
    if (db.open()) {
        qWarning() << "Database" << dbConfig->configuredDatabase()->databaseName() << "already exists, the test is not running in a clean environment!";
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

    const QString socketFile = basePath() + QLatin1String("/local/share/akonadi/akonadiserver.socket");

    qDebug() << "==== Fake Akonadi Server started ====";
    return true;
}

bool FakeAkonadiServer::deleteDirectory(const QString &path)
{
    // TODO: Qt 5: Use QDir::removeRecursively

    Q_ASSERT(path.startsWith(basePath()));
    QDir dir(path);
    dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);

    const QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i).isDir() && !list.at(i).isSymLink()) {
            deleteDirectory(list.at(i).absoluteFilePath());
            const QDir tmpDir(list.at(i).absoluteDir());
            tmpDir.rmdir(list.at(i).fileName());
        } else {
            QFile::remove(list.at(i).absoluteFilePath());
        }
    }
    dir.cdUp();
    dir.rmdir(path);
    return true;
}

bool FakeAkonadiServer::quit()
{
    qDebug() << "==== Fake Akonadi Server shutting down ====";

    // Stop listening for connections
    close();

    const QCommandLineParser &args = AkApplication::instance()->commandLineArguments();
    if (!args.isSet(QLatin1String("no-cleanup"))) {
        deleteDirectory(basePath());
        qDebug() << "Cleaned up" << basePath();
    } else {
        qDebug() << "Skipping clean up of" << basePath();
    }

    PreprocessorManager::done();
    SearchManager::instance();

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

void FakeAkonadiServer::incomingConnection(quintptr socketDescriptor)
{
    QThread *thread = new QThread();
    FakeConnection *connection = new FakeConnection(socketDescriptor, thread);
    thread->start();

    connect(connection, SIGNAL(disconnected()), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), connection, SLOT(deleteLater()));

    mNotificationSpy = new QSignalSpy(connection->notificationCollector(),
                                      SIGNAL(notify(Akonadi::Protocol::ChangeNotification::List)));
    Q_ASSERT(mNotificationSpy->isValid());
}

void FakeAkonadiServer::runTest()
{
    QVERIFY(listen(socketFile()));

    mServerLoop = new QEventLoop(this);
    connect(mClient, SIGNAL(finished()), mServerLoop, SLOT(quit()));

    // Start the client: the client will connect to the server and will
    // start playing the scenario
    mClient->start();

    // Wait until the client disconnects, i.e. until the scenario is completed.
    mServerLoop->exec();

    mServerLoop->deleteLater();
    mServerLoop = 0;

    close();
}

FakeDataStore *FakeAkonadiServer::dataStore() const
{
    Q_ASSERT_X(mDataStore, "FakeAkonadiServer::dataStore()",
               "You have to call FakeAkonadiServer::start() first");
    return mDataStore;
}

QSignalSpy *FakeAkonadiServer::notificationSpy() const
{
    return mNotificationSpy;
}

void FakeAkonadiServer::setPopulateDb(bool populate)
{
    mPopulateDb = populate;
}
