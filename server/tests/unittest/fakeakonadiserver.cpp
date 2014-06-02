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

#include <libs/xdgbasedirs_p.h>
#include <imapparser_p.h>
#include <shared/akstandarddirs.h>
#include <akapplication.h>
#include <storage/dbconfig.h>
#include <storage/datastore.h>
#include <preprocessormanager.h>
#include <search/searchmanager.h>
#include <utils.h>

using namespace Akonadi;
using namespace Akonadi::Server;

FakeAkonadiServer *FakeAkonadiServer::instance()
{
    if (!AkonadiServer::s_instance) {
        AkonadiServer::s_instance = new FakeAkonadiServer;
    }

    Q_ASSERT(qobject_cast<FakeAkonadiServer*>(AkonadiServer::s_instance));
    return qobject_cast<FakeAkonadiServer*>(AkonadiServer::s_instance);
}


FakeAkonadiServer::FakeAkonadiServer()
    : AkonadiServer()
    , mDataStore(0)
    , mServerLoop(0)
    , mNotificationSpy(0)
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

QList<QByteArray> FakeAkonadiServer::loginScenario()
{
    QList<QByteArray> scenario;
    // FIXME: Use real protocol version
    scenario << "S: * OK Akonadi Almost IMAP Server [PROTOCOL " + QByteArray::number(Connection::protocolVersion()) + "]";
    scenario << "C: 0 LOGIN " + instanceName().toLatin1();
    scenario << "S: 0 OK User logged in";
    return scenario;
}

QList<QByteArray> FakeAkonadiServer::defaultScenario()
{
    QList<QByteArray> caps;
    caps << "NOTIFY 2";
    caps << "NOPAYLOADPATH";
    caps << "AKAPPENDSTREAMING";
    return customCapabilitiesScenario(caps);
}

QList<QByteArray> FakeAkonadiServer::customCapabilitiesScenario(const QList<QByteArray> &capabilities)
{
    QList<QByteArray> scenario = loginScenario();
    scenario << "C: 1 CAPABILITY (" + ImapParser::join(capabilities, " ") + ")";
    scenario << "S: 1 OK CAPABILITY completed";
    return scenario;
}

QList<QByteArray> FakeAkonadiServer::selectCollectionScenario(const QString &name)
{
    QList<QByteArray> scenario;
    const Collection collection = Collection::retrieveByName(name);
    scenario << "C: 3 UID SELECT SILENT " + QByteArray::number(collection.id());
    scenario << "S: 3 OK Completed";
    return scenario;
}

QList<QByteArray> FakeAkonadiServer::selectResourceScenario(const QString &name)
{
    QList<QByteArray> scenario;
    const Resource resource = Resource::retrieveByName(name);
    scenario << "C: 2 RESSELECT " + resource.name().toLatin1();
    scenario << "S: 2 OK " + resource.name().toLatin1() + " selected";
    return scenario;
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
        db.close();
    } else {
        db.close();
        db.setDatabaseName(QString());
        if (db.open()) {
            {
                QSqlQuery query(db);
                if (!query.exec(QString::fromLatin1("CREATE DATABASE %1").arg(DbConfig::configuredDatabase()->databaseName()))) {
                    throw FakeAkonadiServerException("Failed to create new database");
                }
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(initCon);

    dbConfig->setup();

    mDataStore = static_cast<FakeDataStore*>(FakeDataStore::self());
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

    const boost::program_options::variables_map options = AkApplication::instance()->commandLineArguments();
    if (!options.count("no-cleanup")) {
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

void FakeAkonadiServer::setScenario(const QList<QByteArray> &scenario)
{
    mClient->setScenario(scenario);
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
                                      SIGNAL(notify(Akonadi::NotificationMessageV3::List)));
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

FakeDataStore* FakeAkonadiServer::dataStore() const
{
    Q_ASSERT_X(mDataStore, "FakeAkonadiServer::connection()",
               "You have to call FakeAkonadiServer::start() first");
    return mDataStore;
}

QSignalSpy* FakeAkonadiServer::notificationSpy() const
{
    return mNotificationSpy;
}
