/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbaccess.h"

#include <Akonadi/ServerManager>

#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include <KMessageBox>

using namespace Akonadi;

class DbAccessPrivate
{
public:
    DbAccessPrivate()
    {
        init();
    }

    void init()
    {
        const QString serverConfigFile = ServerManager::serverConfigFilePath(ServerManager::ReadWrite);
        QSettings settings(serverConfigFile, QSettings::IniFormat);

        const QString driver = settings.value(QStringLiteral("General/Driver"), QStringLiteral("QMYSQL")).toString();
        database = QSqlDatabase::addDatabase(driver);
        settings.beginGroup(driver);
        database.setHostName(settings.value(QStringLiteral("Host"), QString()).toString());
        database.setDatabaseName(settings.value(QStringLiteral("Name"), QStringLiteral("akonadi")).toString());
        database.setUserName(settings.value(QStringLiteral("User"), QString()).toString());
        database.setPassword(settings.value(QStringLiteral("Password"), QString()).toString());
        database.setConnectOptions(settings.value(QStringLiteral("Options"), QString()).toString());
        if (!database.open()) {
            KMessageBox::error(nullptr, QStringLiteral("Failed to connect to database: %1").arg(database.lastError().text()));
        }
    }

    QSqlDatabase database;
};

Q_GLOBAL_STATIC(DbAccessPrivate, sInstance)
QSqlDatabase DbAccess::database()
{
    // hack to detect database gone away error
    QSqlQuery query(sInstance->database);
    // prepare or exec of "SELECT 1" will only fail when we are not connected to database
    if (!query.prepare(QStringLiteral("SELECT 1")) || !query.exec()) {
        sInstance->database.close();
        QSqlDatabase::removeDatabase(sInstance->database.connectionName());
        sInstance->init();
    }
    return sInstance->database;
}
