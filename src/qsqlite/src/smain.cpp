/****************************************************************************
**
** SPDX-FileCopyrightText: 2009 Nokia Corporation and /or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** SPDX-FileCopyrightText: LGPL-2.1-only WITH Qt-LGPL-exception-1.1 OR GPL-3.0-only
**
****************************************************************************/

#include <QtCore/QStringList>
#include <QtSql/QSqlDriverPlugin>

#include "qsql_sqlite.h"

QT_BEGIN_NAMESPACE

class QSQLiteDriverPlugin : public QSqlDriverPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSqlDriverFactoryInterface" FILE "sqlite3.json")

public:
    QSQLiteDriverPlugin();

    QSqlDriver *create(const QString &) override;
};

QSQLiteDriverPlugin::QSQLiteDriverPlugin()
    : QSqlDriverPlugin()
{
}

QSqlDriver *QSQLiteDriverPlugin::create(const QString &name)
{
    if (name == QLatin1String("QSQLITE3")) {
        auto driver = new QSQLiteDriver();
        return driver;
    }
    return nullptr;
}

#include "smain.moc"

QT_END_NAMESPACE
