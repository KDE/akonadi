/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#include "akonadi.h"
#include "handler.h"
#include "connection.h"
#include "serveradaptor.h"
#include "akonadiserver_debug.h"

#include "cachecleaner.h"
#include "intervalcheck.h"
#include "storagejanitor.h"
#include "storage/dbconfig.h"
#include "storage/datastore.h"
#include "notificationmanager.h"
#include "resourcemanager.h"
#include "tracer.h"
#include "utils.h"
#include "debuginterface.h"
#include "storage/itemretrievalmanager.h"
#include "storage/collectionstatistics.h"
#include "preprocessormanager.h"
#include "search/searchmanager.h"
#include "search/searchtaskmanager.h"
#include "aklocalserver.h"

#include <private/standarddirs_p.h>
#include <private/protocol_p.h>
#include <private/dbus_p.h>
#include <private/instance_p.h>

#include <QSqlQuery>
#include <QSqlError>

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QTimer>
#include <QDBusServiceWatcher>

using namespace Akonadi;
using namespace Akonadi::Server;

AkonadiServer *AkonadiServer::s_instance = nullptr;

AkonadiServer::AkonadiServer(QObject *parent)
    : QObject(parent)
{
    // Register bunch of useful types
    qRegisterMetaType<Protocol::CommandPtr>();
    qRegisterMetaType<Protocol::ChangeNotificationPtr>();
    qRegisterMetaType<Protocol::ChangeNotificationList>();
    qRegisterMetaType<quintptr>("quintptr");
}

bool AkonadiServer::init()
{
    qCInfo(AKONADISERVER_LOG) << "Starting up the Akonadi Server...";

    const QString serverConfigFile = StandardDirs::serverConfigFile(StandardDirs::ReadWrite);
    QSettings settings(serverConfigFile, QSettings::IniFormat);
    // Restrict permission to 600, as the file might contain database password in plaintext
    QFile::setPermissions(serverConfigFile, QFile::ReadOwner | QFile::WriteOwner);

    if (!DbConfig::configuredDatabase()) {
        quit();
        return false;
    }

    if (DbConfig::configuredDatabase()->useInternalServer()) {
        if (!startDatabaseProcess()) {
            quit();
            return false;
        }
    } else {
        if (!createDatabase()) {
            quit();
            return false;
        }
    }

    DbConfig::configuredDatabase()->setup();

    s_instance = this;

    const QString connectionSettingsFile = StandardDirs::connectionConfigFile(StandardDirs::WriteOnly);
    QSettings connectionSettings(connectionSettingsFile, QSettings::IniFormat);

    mCmdServer = new AkLocalServer(this);
    connect(mCmdServer, QOverload<quintptr>::of(&AkLocalServer::newConnection), this, &AkonadiServer::newCmdConnection);

    mNotificationManager = new NotificationManager();
    mNtfServer = new AkLocalServer(this);
    // Note: this is a queued connection, as NotificationManager lives in its
    // own thread
    connect(mNtfServer, QOverload<quintptr>::of(&AkLocalServer::newConnection), mNotificationManager, &NotificationManager::registerConnection);

    // TODO: share socket setup with client
#ifdef Q_OS_WIN
    // use the installation prefix as uid
    QString suffix;
    if (Instance::hasIdentifier()) {
        suffix = QStringLiteral("%1-").arg(Instance::identifier());
    }
    suffix += QString::fromUtf8(QUrl::toPercentEncoding(qApp->applicationDirPath()));
    const QString defaultCmdPipe = QStringLiteral("Akonadi-Cmd-") % suffix;
    const QString cmdPipe = settings.value(QStringLiteral("Connection/NamedPipe"), defaultCmdPipe).toString();
    if (!mCmdServer->listen(cmdPipe)) {
        qCCritical(AKONADISERVER_LOG) << "Unable to listen on Named Pipe" << cmdPipe << ":" << mCmdServer->errorString();
        quit();
        return false;
    }

    const QString defaultNtfPipe = QStringLiteral("Akonadi-Ntf-") % suffix;
    const QString ntfPipe = settings.value(QStringLiteral("Connection/NtfNamedPipe"), defaultNtfPipe).toString();
    if (!mNtfServer->listen(ntfPipe)) {
        qCCritical(AKONADISERVER_LOG) << "Unable to listen on Named Pipe" << ntfPipe << ":" << mNtfServer->errorString();
        quit();
        return false;
    }

    connectionSettings.setValue(QStringLiteral("Data/Method"), QStringLiteral("NamedPipe"));
    connectionSettings.setValue(QStringLiteral("Data/NamedPipe"), cmdPipe);
    connectionSettings.setValue(QStringLiteral("Notifications/Method"), QStringLiteral("NamedPipe"));
    connectionSettings.setValue(QStringLiteral("Notifications/NamedPipe"), ntfPipe);
#else
    const QString cmdSocketName = QStringLiteral("akonadiserver-cmd.socket");
    const QString ntfSocketName = QStringLiteral("akonadiserver-ntf.socket");
    const QString socketDir = Utils::preferredSocketDirectory(StandardDirs::saveDir("data"),
                                                              qMax(cmdSocketName.length(), ntfSocketName.length()));
    const QString cmdSocketFile = socketDir % QLatin1Char('/') % cmdSocketName;
    QFile::remove(cmdSocketFile);
    if (!mCmdServer->listen(cmdSocketFile)) {
        qCCritical(AKONADISERVER_LOG) << "Unable to listen on Unix socket" << cmdSocketFile << ":" << mCmdServer->errorString();
        quit();
        return false;
    }

    const QString ntfSocketFile = socketDir % QLatin1Char('/') % ntfSocketName;
    QFile::remove(ntfSocketFile);
    if (!mNtfServer->listen(ntfSocketFile)) {
        qCCritical(AKONADISERVER_LOG) << "Unable to listen on Unix socket" << ntfSocketFile << ":" << mNtfServer->errorString();
        quit();
        return false;
    }

    connectionSettings.setValue(QStringLiteral("Data/Method"), QStringLiteral("UnixPath"));
    connectionSettings.setValue(QStringLiteral("Data/UnixPath"), cmdSocketFile);
    connectionSettings.setValue(QStringLiteral("Notifications/Method"), QStringLiteral("UnixPath"));
    connectionSettings.setValue(QStringLiteral("Notifications/UnixPath"), ntfSocketFile);
#endif

    // initialize the database
    DataStore *db = DataStore::self();
    if (!db->database().isOpen()) {
        qCCritical(AKONADISERVER_LOG) << "Unable to open database" << db->database().lastError().text();
        quit();
        return false;
    }
    if (!db->init()) {
        qCCritical(AKONADISERVER_LOG) << "Unable to initialize database.";
        quit();
        return false;
    }

    Tracer::self();
    new DebugInterface(this);
    ResourceManager::self();

    CollectionStatistics::self();

    // Initialize the preprocessor manager
    PreprocessorManager::init();

    // Forcibly disable it if configuration says so
    if (settings.value(QStringLiteral("General/DisablePreprocessing"), false).toBool()) {
        PreprocessorManager::instance()->setEnabled(false);
    }

    if (settings.value(QStringLiteral("Cache/EnableCleaner"), true).toBool()) {
        mCacheCleaner = new CacheCleaner();
    }

    mIntervalCheck = new IntervalCheck();
    mStorageJanitor = new StorageJanitor();
    mItemRetrieval = new ItemRetrievalManager();
    mAgentSearchManager = new SearchTaskManager();

    const QStringList searchManagers = settings.value(QStringLiteral("Search/Manager"),
                                       QStringList() << QStringLiteral("Agent")).toStringList();
    mSearchManager = new SearchManager(searchManagers);

    new ServerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Server"), this);

    const QByteArray dbusAddress = qgetenv("DBUS_SESSION_BUS_ADDRESS");
    if (!dbusAddress.isEmpty()) {
        connectionSettings.setValue(QStringLiteral("DBUS/Address"), QLatin1String(dbusAddress));
    }

    mControlWatcher = std::make_unique<QDBusServiceWatcher>(
            DBus::serviceName(DBus::Control), QDBusConnection::sessionBus(),
            QDBusServiceWatcher::WatchForUnregistration);
    connect(mControlWatcher.get(), &QDBusServiceWatcher::serviceUnregistered,
            this, [this]() {
                qCCritical(AKONADISERVER_LOG) << "Control process died, committing suicide!";
                quit();
            });

    // Unhide all the items that are actually hidden.
    // The hidden flag was probably left out after an (abrupt)
    // server quit. We don't attempt to resume preprocessing
    // for the items as we don't actually know at which stage the
    // operation was interrupted...
    db->unhideAllPimItems();

    // We are ready, now register org.freedesktop.Akonadi service to DBus and
    // the fun can begin
    if (!QDBusConnection::sessionBus().registerService(DBus::serviceName(DBus::Server))) {
        qCCritical(AKONADISERVER_LOG) << "Unable to connect to dbus service: " << QDBusConnection::sessionBus().lastError().message();
        quit();
        return false;
    }

    return true;
}

AkonadiServer::~AkonadiServer()
{
}

template <typename T> static void quitThread(T &thread)
{
    if (thread) {
        thread->quit();
        thread->wait();
        delete thread;
        thread = nullptr;
    }
}

bool AkonadiServer::quit()
{
    if (mAlreadyShutdown) {
        return true;
    }
    mAlreadyShutdown = true;

    qCDebug(AKONADISERVER_LOG) << "terminating connection threads";
    qDeleteAll(mConnections);
    mConnections.clear();

    qCDebug(AKONADISERVER_LOG) << "terminating service threads";
    delete mCacheCleaner;
    delete mIntervalCheck;
    delete mStorageJanitor;
    delete mItemRetrieval;
    delete mAgentSearchManager;
    delete mSearchManager;
    delete mNotificationManager;

    // Terminate the preprocessor manager before the database but after all connections are gone
    PreprocessorManager::done();
    CollectionStatistics::destroy();

    if (DbConfig::isConfigured()) {
        if (DataStore::hasDataStore()) {
            DataStore::self()->close();
        }
        qCDebug(AKONADISERVER_LOG) << "stopping db process";
        stopDatabaseProcess();
    }

    //QSettings settings(StandardDirs::serverConfigFile(), QSettings::IniFormat);
    const QString connectionSettingsFile = StandardDirs::connectionConfigFile(StandardDirs::WriteOnly);

    if (!QDir::home().remove(connectionSettingsFile)) {
        qCCritical(AKONADISERVER_LOG) << "Failed to remove runtime connection config file";
    }

    QTimer::singleShot(0, this, &AkonadiServer::doQuit);

    return true;
}

void AkonadiServer::doQuit()
{
    QCoreApplication::exit();
}

void AkonadiServer::newCmdConnection(quintptr socketDescriptor)
{
    if (mAlreadyShutdown) {
        return;
    }

    Connection *connection = new Connection(socketDescriptor);
    connect(connection, &Connection::disconnected,
            this, &AkonadiServer::connectionDisconnected);
    mConnections.append(connection);
}

void AkonadiServer::connectionDisconnected()
{
    auto conn = qobject_cast<Connection *>(sender());
    mConnections.removeOne(conn);
    delete conn;
}

AkonadiServer *AkonadiServer::instance()
{
    if (!s_instance) {
        s_instance = new AkonadiServer();
    }
    return s_instance;
}

bool AkonadiServer::startDatabaseProcess()
{
    if (!DbConfig::configuredDatabase()->useInternalServer()) {
        qCCritical(AKONADISERVER_LOG) << "Trying to start external database!";
    }

    // create the database directories if they don't exists
    StandardDirs::saveDir("data");
    StandardDirs::saveDir("data", QStringLiteral("file_db_data"));

    return DbConfig::configuredDatabase()->startInternalServer();
}

bool AkonadiServer::createDatabase()
{
    bool success = true;
    const QLatin1String initCon("initConnection");
    QSqlDatabase db = QSqlDatabase::addDatabase(DbConfig::configuredDatabase()->driverName(), initCon);
    DbConfig::configuredDatabase()->apply(db);
    db.setDatabaseName(DbConfig::configuredDatabase()->databaseName());
    if (!db.isValid()) {
        qCCritical(AKONADISERVER_LOG) << "Invalid database object during initial database connection";
        return false;
    }

    if (db.open()) {
        db.close();
    } else {
        qCCritical(AKONADISERVER_LOG) << "Failed to use database" << DbConfig::configuredDatabase()->databaseName();
        qCCritical(AKONADISERVER_LOG) << "Database error:" << db.lastError().text();
        qCDebug(AKONADISERVER_LOG) << "Trying to create database now...";

        db.close();
        db.setDatabaseName(QString());
        if (db.open()) {
            {
                QSqlQuery query(db);
                if (!query.exec(QStringLiteral("CREATE DATABASE %1").arg(DbConfig::configuredDatabase()->databaseName()))) {
                    qCCritical(AKONADISERVER_LOG) << "Failed to create database";
                    qCCritical(AKONADISERVER_LOG) << "Query error:" << query.lastError().text();
                    qCCritical(AKONADISERVER_LOG) << "Database error:" << db.lastError().text();
                    success = false;
                }
            } // make sure query is destroyed before we close the db
            db.close();
        } else {
            qCCritical(AKONADISERVER_LOG) << "Failed to connect to database!";
            qCCritical(AKONADISERVER_LOG) << "Database error:" << db.lastError().text();
            success = false;
        }
    }
    return success;
}

void AkonadiServer::stopDatabaseProcess()
{
    if (!DbConfig::configuredDatabase()->useInternalServer()) {
        // closing initConnection this late to work around QTBUG-63108
        QSqlDatabase::removeDatabase(QStringLiteral("initConnection"));
        return;
    }

    DbConfig::configuredDatabase()->stopInternalServer();
}

CacheCleaner *AkonadiServer::cacheCleaner()
{
    return mCacheCleaner;
}

IntervalCheck *AkonadiServer::intervalChecker()
{
    return mIntervalCheck;
}

NotificationManager *AkonadiServer::notificationManager()
{
    return mNotificationManager;
}

QString AkonadiServer::serverPath() const
{
    return StandardDirs::saveDir("config");
}
