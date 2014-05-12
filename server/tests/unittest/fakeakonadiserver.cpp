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
#include "fakedatastore.h"

#include <QSettings>
#include <QCoreApplication>
#include <QSqlQuery>
#include <QDir>
#include <QFileInfo>

#include <shared/akstandarddirs.h>
#include <libs/xdgbasedirs_p.h>
#include <storage/dbconfig.h>
#include <storage/datastore.h>


using namespace Akonadi;
using namespace Akonadi::Server;

FakeAkonadiServer::FakeAkonadiServer()
    : mConnection(0)
    , mDataStore(0)
{
}

FakeAkonadiServer::~FakeAkonadiServer()
{
    cleanup();
}

bool FakeAkonadiServer::abortSetup(const QString &msg)
{
    qWarning() << msg.toUtf8().constData();
    qWarning() << "Aborting setup...";
    return false;
}


QString FakeAkonadiServer::basePath() const
{
    return QString::fromLatin1("/tmp/akonadiserver-test-%1").arg(QCoreApplication::instance()->applicationPid());
}

QString FakeAkonadiServer::instanceName() const
{
    return QString::fromLatin1("akonadiserver-test-%1").arg(QCoreApplication::instance()->applicationPid());
}

bool FakeAkonadiServer::start()
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
      return abortSetup(QLatin1String("Unexpected driver specified. Expected QSQLITE3, got ") + dbConfig->driverName());
    }

    const QLatin1String initCon("initConnection");
    QSqlDatabase db = QSqlDatabase::addDatabase(DbConfig::configuredDatabase()->driverName(), initCon);
    DbConfig::configuredDatabase()->apply(db);
    db.setDatabaseName(DbConfig::configuredDatabase()->databaseName());
    if (!db.isDriverAvailable(DbConfig::configuredDatabase()->driverName())) {
        return abortSetup(QString::fromLatin1("SQL driver %s not available").arg(db.driverName()));
    }
    if (!db.isValid()) {
        return abortSetup(QLatin1String("Got invalid database"));
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
                    return abortSetup(QLatin1String("Failed to create new database"));
                }
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(initCon);

    dbConfig->setup();

    mDataStore = static_cast<FakeDataStore*>(FakeDataStore::self());
    if (!mDataStore->init()) {
      return abortSetup(QLatin1String("Failed to initialize datastore"));
    }

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


bool FakeAkonadiServer::cleanup()
{
    qDebug() << "==== Fake Akonadi Server shutting down ====";

    deleteDirectory(basePath());
    qDebug() << "Cleaned up" << basePath();


    qDebug() << "==== Fake Akonadi Server shut down ====";
    return true;
}

FakeConnection* FakeAkonadiServer::connection(Handler *handler, const QByteArray &command,
                                              const CommandContext &context)
{
    mConnection = new FakeConnection();
    mConnection->setHandler(handler);
    mConnection->setCommand(command);
    mConnection->setContext(context);

    return mConnection;
}

FakeDataStore* FakeAkonadiServer::dataStore()
{
    Q_ASSERT_X(mDataStore, "FakeAkonadiServer::connection()",
               "You have to call FakeAkonadiServer::start() first");
    return mDataStore;
}

