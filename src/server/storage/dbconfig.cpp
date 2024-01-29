/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbconfig.h"

#include "akonadiserver_debug.h"
#include "dbconfigmysql.h"
#include "dbconfigpostgresql.h"
#include "dbconfigsqlite.h"

#include <config-akonadi.h>

#include <private/instance_p.h>
#include <private/standarddirs_p.h>

#include <QProcess>
#include <memory>

using namespace Akonadi;
using namespace Akonadi::Server;

// TODO: make me Q_GLOBAL_STATIC
static DbConfig *s_DbConfigInstance = nullptr;

DbConfig::DbConfig()
    : DbConfig(StandardDirs::serverConfigFile(StandardDirs::ReadWrite))
{
}

DbConfig::DbConfig(const QString &configFile)
{
    QSettings settings(configFile, QSettings::IniFormat);

    mSizeThreshold = 4096;
    const QVariant value = settings.value(QStringLiteral("General/SizeThreshold"), mSizeThreshold);
    if (value.canConvert<qint64>()) {
        mSizeThreshold = value.value<qint64>();
        if (mSizeThreshold < 0) {
            mSizeThreshold = 0;
        }
    } else {
        mSizeThreshold = 0;
    }
}

DbConfig::~DbConfig()
{
}

bool DbConfig::isConfigured()
{
    return s_DbConfigInstance;
}

QString DbConfig::defaultAvailableDatabaseBackend(QSettings &settings)
{
    QString driverName = QStringLiteral(AKONADI_DATABASE_BACKEND);

    std::unique_ptr<DbConfig> dbConfigFallbackTest;
    if (driverName == QLatin1StringView("QMYSQL")) {
        dbConfigFallbackTest = std::make_unique<DbConfigMysql>();
    } else if (driverName == QLatin1StringView("QPSQL")) {
        dbConfigFallbackTest = std::make_unique<DbConfigPostgresql>();
    }

    if (dbConfigFallbackTest && !dbConfigFallbackTest->isAvailable(settings) && DbConfigSqlite().isAvailable(settings)) {
        qCWarning(AKONADISERVER_LOG) << driverName << " requirements not available. Falling back to using QSQLITE.";
        driverName = QStringLiteral("QSQLITE");
    }

    return driverName;
}

DbConfig *DbConfig::configuredDatabase()
{
    if (!s_DbConfigInstance) {
        const QString serverConfigFile = StandardDirs::serverConfigFile(StandardDirs::ReadWrite);
        QSettings settings(serverConfigFile, QSettings::IniFormat);

        // determine driver to use
        QString driverName = settings.value(QStringLiteral("General/Driver")).toString();
        if (driverName.isEmpty()) {
            driverName = defaultAvailableDatabaseBackend(settings);

            // when using the default, write it explicitly, in case the default changes later
            settings.setValue(QStringLiteral("General/Driver"), driverName);
            settings.sync();
        }

        if (driverName == QLatin1StringView("QMYSQL")) {
            s_DbConfigInstance = new DbConfigMysql;
        } else if (driverName == QLatin1StringView("QSQLITE") || driverName == QLatin1String("QSQLITE3")) {
            // QSQLITE3 is legacy name for the Akonadi fork of the upstream QSQLITE driver.
            // It is kept here for backwards compatibility with old server config files.
            s_DbConfigInstance = new DbConfigSqlite();
        } else if (driverName == QLatin1StringView("QPSQL")) {
            s_DbConfigInstance = new DbConfigPostgresql;
        } else {
            qCCritical(AKONADISERVER_LOG) << "Unknown database driver: " << driverName;
            qCCritical(AKONADISERVER_LOG) << "Available drivers are: " << QSqlDatabase::drivers();
            return nullptr;
        }

        if (!s_DbConfigInstance->init(settings)) {
            delete s_DbConfigInstance;
            s_DbConfigInstance = nullptr;
        }
    }

    return s_DbConfigInstance;
}

void DbConfig::destroy()
{
    delete s_DbConfigInstance;
    s_DbConfigInstance = nullptr;
}

bool DbConfig::startInternalServer()
{
    // do nothing
    return true;
}

void DbConfig::stopInternalServer()
{
    // do nothing
}

void DbConfig::setup()
{
    // do nothing
}

qint64 DbConfig::sizeThreshold() const
{
    return mSizeThreshold;
}

QString DbConfig::defaultDatabaseName()
{
    if (!Instance::hasIdentifier()) {
        return QStringLiteral("akonadi");
    }
    // dash is not allowed in PSQL
    return QLatin1StringView("akonadi_") % Instance::identifier().replace(QLatin1Char('-'), QLatin1Char('_'));
}

void DbConfig::initSession(const QSqlDatabase &database)
{
    Q_UNUSED(database)
}

int DbConfig::execute(const QString &cmd, const QStringList &args) const
{
    qCDebug(AKONADISERVER_LOG) << "Executing: " << cmd << args.join(QLatin1Char(' '));
    return QProcess::execute(cmd, args);
}
