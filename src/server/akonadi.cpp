/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>                 *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "akonadi.h"
#include "akonadiserver_debug.h"
#include "connection.h"
#include "handler.h"
#include "serveradaptor.h"

#include "aklocalserver.h"
#include "cachecleaner.h"
#include "debuginterface.h"
#include "intervalcheck.h"
#include "notificationmanager.h"
#include "preprocessormanager.h"
#include "resourcemanager.h"
#include "search/searchmanager.h"
#include "search/searchtaskmanager.h"
#include "storage/collectionstatistics.h"
#include "storage/datastore.h"
#include "storage/dbconfig.h"
#include "storage/itemretrievalmanager.h"
#include "storagejanitor.h"
#include "tracer.h"
#include "utils.h"

#include <private/dbus_p.h>
#include <private/instance_p.h>
#include <private/protocol_p.h>
#include <private/standarddirs_p.h>

#include <QSqlError>
#include <QSqlQuery>

#include <QCoreApplication>
#include <QDBusServiceWatcher>
#include <QDir>
#include <QSettings>
#include <QTimer>

using namespace Akonadi;
using namespace Akonadi::Server;
using namespace std::chrono_literals;
namespace
{
class AkonadiDataStore : public DataStore
{
    Q_OBJECT
public:
    explicit AkonadiDataStore(AkonadiServer *server)
        : DataStore(server, DbConfig::configuredDatabase())
    {
    }
};

class AkonadiDataStoreFactory : public DataStoreFactory
{
public:
    explicit AkonadiDataStoreFactory(AkonadiServer *akonadi)
        : m_akonadi(akonadi)
    {
    }

    DataStore *createStore() override
    {
        return new AkonadiDataStore(m_akonadi);
    }

private:
    AkonadiServer *const m_akonadi;
};

} // namespace

AkonadiServer::AkonadiServer()
{
    // Register bunch of useful types
    qRegisterMetaType<Protocol::CommandPtr>();
    qRegisterMetaType<Protocol::ChangeNotificationPtr>();
    qRegisterMetaType<Protocol::ChangeNotificationList>();
    qRegisterMetaType<quintptr>("quintptr");

    DataStore::setFactory(std::make_unique<AkonadiDataStoreFactory>(this));
}

bool AkonadiServer::init()
{
    qCInfo(AKONADISERVER_LOG) << "Starting up the Akonadi Server...";

    const QString serverConfigFile = StandardDirs::serverConfigFile(StandardDirs::ReadWrite);
    QSettings settings(serverConfigFile, QSettings::IniFormat);
    // Restrict permission to 600, as the file might contain database password in plaintext
    QFile::setPermissions(serverConfigFile, QFile::ReadOwner | QFile::WriteOwner);

    const QString connectionSettingsFile = StandardDirs::connectionConfigFile(StandardDirs::WriteOnly);
    QSettings connectionSettings(connectionSettingsFile, QSettings::IniFormat);

    const QByteArray dbusAddress = qgetenv("DBUS_SESSION_BUS_ADDRESS");
    if (!dbusAddress.isEmpty()) {
        connectionSettings.setValue(QStringLiteral("DBUS/Address"), QLatin1String(dbusAddress));
    }

    // Setup database
    if (!setupDatabase()) {
        quit();
        return false;
    }

    // Create local servers and start listening
    if (!createServers(settings, connectionSettings)) {
        quit();
        return false;
    }

    const auto searchManagers = settings.value(QStringLiteral("Search/Manager"), QStringList{QStringLiteral("Agent")}).toStringList();

    mTracer = std::make_unique<Tracer>();
    mCollectionStats = std::make_unique<CollectionStatistics>();
    mCacheCleaner = AkThread::create<CacheCleaner>();
    mItemRetrieval = AkThread::create<ItemRetrievalManager>();
    mAgentSearchManager = AkThread::create<SearchTaskManager>();

    mDebugInterface = std::make_unique<DebugInterface>(*mTracer);
    mResourceManager = std::make_unique<ResourceManager>(*mTracer);
    mPreprocessorManager = std::make_unique<PreprocessorManager>(*mTracer);
    mIntervalCheck = AkThread::create<IntervalCheck>(*mItemRetrieval);
    mSearchManager = AkThread::create<SearchManager>(searchManagers, *mAgentSearchManager);
    mStorageJanitor = AkThread::create<StorageJanitor>(*this);

    if (settings.value(QStringLiteral("General/DisablePreprocessing"), false).toBool()) {
        mPreprocessorManager->setEnabled(false);
    }

    new ServerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Server"), this);

    mControlWatcher =
        std::make_unique<QDBusServiceWatcher>(DBus::serviceName(DBus::Control), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForUnregistration);
    connect(mControlWatcher.get(), &QDBusServiceWatcher::serviceUnregistered, this, [this]() {
        qCCritical(AKONADISERVER_LOG) << "Control process died, exiting!";
        quit();
    });

    // Unhide all the items that are actually hidden.
    // The hidden flag was probably left out after an (abrupt)
    // server quit. We don't attempt to resume preprocessing
    // for the items as we don't actually know at which stage the
    // operation was interrupted...
    DataStore::self()->unhideAllPimItems();

    // We are ready, now register org.freedesktop.Akonadi service to DBus and
    // the fun can begin
    if (!QDBusConnection::sessionBus().registerService(DBus::serviceName(DBus::Server))) {
        qCCritical(AKONADISERVER_LOG) << "Unable to connect to dbus service: " << QDBusConnection::sessionBus().lastError().message();
        quit();
        return false;
    }

    return true;
}

AkonadiServer::~AkonadiServer() = default;

bool AkonadiServer::quit()
{
    if (mAlreadyShutdown) {
        return true;
    }
    mAlreadyShutdown = true;

    qCDebug(AKONADISERVER_LOG) << "terminating connection threads";
    mConnections.clear();

    qCDebug(AKONADISERVER_LOG) << "terminating service threads";
    // Keep this order in sync (reversed) with the order of initialization
    mStorageJanitor.reset();
    mSearchManager.reset();
    mIntervalCheck.reset();
    mPreprocessorManager.reset();
    mResourceManager.reset();
    mDebugInterface.reset();

    mAgentSearchManager.reset();
    mItemRetrieval.reset();
    mCacheCleaner.reset();
    mCollectionStats.reset();
    mTracer.reset();

    if (DbConfig::isConfigured()) {
        if (DataStore::hasDataStore()) {
            DataStore::self()->close();
        }
        qCDebug(AKONADISERVER_LOG) << "stopping db process";
        stopDatabaseProcess();
    }

    // QSettings settings(StandardDirs::serverConfigFile(), QSettings::IniFormat);
    const QString connectionSettingsFile = StandardDirs::connectionConfigFile(StandardDirs::WriteOnly);

    if (!QDir::home().remove(connectionSettingsFile)) {
        qCCritical(AKONADISERVER_LOG) << "Failed to remove runtime connection config file";
    }

    QTimer::singleShot(0s, this, &AkonadiServer::doQuit);

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

    auto connection = AkThread::create<Connection>(socketDescriptor, *this);
    connect(connection.get(), &Connection::disconnected, this, &AkonadiServer::connectionDisconnected);
    mConnections.push_back(std::move(connection));
}

void AkonadiServer::connectionDisconnected()
{
    auto it = std::find_if(mConnections.begin(), mConnections.end(), [this](const auto &ptr) {
        return ptr.get() == sender();
    });

    if (it != mConnections.end()) {
        mConnections.erase(it);
    }
}

bool AkonadiServer::setupDatabase()
{
    if (!DbConfig::configuredDatabase()) {
        return false;
    }

    if (DbConfig::configuredDatabase()->useInternalServer()) {
        if (!startDatabaseProcess()) {
            return false;
        }
    } else {
        if (!createDatabase()) {
            return false;
        }
    }

    DbConfig::configuredDatabase()->setup();

    // initialize the database
    DataStore *db = DataStore::self();
    if (!db->database().isOpen()) {
        qCCritical(AKONADISERVER_LOG) << "Unable to open database" << db->database().lastError().text();
        return false;
    }
    if (!db->init()) {
        qCCritical(AKONADISERVER_LOG) << "Unable to initialize database.";
        return false;
    }

    return true;
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

bool AkonadiServer::createServers(QSettings &settings, QSettings &connectionSettings)
{
    mCmdServer = std::make_unique<AkLocalServer>(this);
    connect(mCmdServer.get(), qOverload<quintptr>(&AkLocalServer::newConnection), this, &AkonadiServer::newCmdConnection);

    mNotificationManager = AkThread::create<NotificationManager>();
    mNtfServer = std::make_unique<AkLocalServer>(this);
    // Note: this is a queued connection, as NotificationManager lives in its
    // own thread
    connect(mNtfServer.get(), qOverload<quintptr>(&AkLocalServer::newConnection), mNotificationManager.get(), &NotificationManager::registerConnection);

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
        return false;
    }

    const QString defaultNtfPipe = QStringLiteral("Akonadi-Ntf-") % suffix;
    const QString ntfPipe = settings.value(QStringLiteral("Connection/NtfNamedPipe"), defaultNtfPipe).toString();
    if (!mNtfServer->listen(ntfPipe)) {
        qCCritical(AKONADISERVER_LOG) << "Unable to listen on Named Pipe" << ntfPipe << ":" << mNtfServer->errorString();
        return false;
    }

    connectionSettings.setValue(QStringLiteral("Data/Method"), QStringLiteral("NamedPipe"));
    connectionSettings.setValue(QStringLiteral("Data/NamedPipe"), cmdPipe);
    connectionSettings.setValue(QStringLiteral("Notifications/Method"), QStringLiteral("NamedPipe"));
    connectionSettings.setValue(QStringLiteral("Notifications/NamedPipe"), ntfPipe);
#else
    Q_UNUSED(settings)

    const QString cmdSocketName = QStringLiteral("akonadiserver-cmd.socket");
    const QString ntfSocketName = QStringLiteral("akonadiserver-ntf.socket");
    const QString socketDir = Utils::preferredSocketDirectory(StandardDirs::saveDir("data"), qMax(cmdSocketName.length(), ntfSocketName.length()));
    const QString cmdSocketFile = socketDir % QLatin1Char('/') % cmdSocketName;
    QFile::remove(cmdSocketFile);
    if (!mCmdServer->listen(cmdSocketFile)) {
        qCCritical(AKONADISERVER_LOG) << "Unable to listen on Unix socket" << cmdSocketFile << ":" << mCmdServer->errorString();
        return false;
    }

    const QString ntfSocketFile = socketDir % QLatin1Char('/') % ntfSocketName;
    QFile::remove(ntfSocketFile);
    if (!mNtfServer->listen(ntfSocketFile)) {
        qCCritical(AKONADISERVER_LOG) << "Unable to listen on Unix socket" << ntfSocketFile << ":" << mNtfServer->errorString();
        return false;
    }

    connectionSettings.setValue(QStringLiteral("Data/Method"), QStringLiteral("UnixPath"));
    connectionSettings.setValue(QStringLiteral("Data/UnixPath"), cmdSocketFile);
    connectionSettings.setValue(QStringLiteral("Notifications/Method"), QStringLiteral("UnixPath"));
    connectionSettings.setValue(QStringLiteral("Notifications/UnixPath"), ntfSocketFile);
#endif

    return true;
}

CacheCleaner *AkonadiServer::cacheCleaner()
{
    return mCacheCleaner.get();
}

IntervalCheck &AkonadiServer::intervalChecker()
{
    return *mIntervalCheck;
}

ResourceManager &AkonadiServer::resourceManager()
{
    return *mResourceManager;
}

NotificationManager *AkonadiServer::notificationManager()
{
    return mNotificationManager.get();
}

CollectionStatistics &AkonadiServer::collectionStatistics()
{
    return *mCollectionStats;
}

PreprocessorManager &AkonadiServer::preprocessorManager()
{
    return *mPreprocessorManager;
}

SearchTaskManager &AkonadiServer::agentSearchManager()
{
    return *mAgentSearchManager;
}

SearchManager &AkonadiServer::searchManager()
{
    return *mSearchManager;
}

ItemRetrievalManager &AkonadiServer::itemRetrievalManager()
{
    return *mItemRetrieval;
}

Tracer &AkonadiServer::tracer()
{
    return *mTracer;
}

QString AkonadiServer::serverPath() const
{
    return StandardDirs::saveDir("config");
}

#include "akonadi.moc"

#include "moc_akonadi.cpp"
