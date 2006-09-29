/****************************************************************************
**
** Copyright (C) 1992-2006 Trolltech ASA. All rights reserved.
**
** This file is part of the QtSql module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QSQL_MYSQLEMBEDDED_H
#define QSQL_MYSQLEMBEDDED_H

#include <QtSql/qsqldriver.h>
#include <QtSql/qsqlresult.h>

#if defined (Q_OS_WIN32)
#include <QtCore/qt_windows.h>
#endif

#include <mysql.h>

#ifdef QT_PLUGIN
#define Q_EXPORT_SQLDRIVER_MYSQL
#else
#define Q_EXPORT_SQLDRIVER_MYSQL Q_SQL_EXPORT
#endif

QT_BEGIN_HEADER

class QMYSQLEmbeddedDriverPrivate;
class QMYSQLEmbeddedResultPrivate;
class QMYSQLEmbeddedDriver;
class QSqlRecordInfo;

class QMYSQLEmbeddedResult : public QSqlResult
{
    friend class QMYSQLEmbeddedDriver;
public:
    explicit QMYSQLEmbeddedResult(const QMYSQLEmbeddedDriver* db);
    ~QMYSQLEmbeddedResult();

    QVariant handle() const;
protected:
    void cleanup();
    bool fetch(int i);
    bool fetchNext();
    bool fetchLast();
    bool fetchFirst();
    QVariant data(int field);
    bool isNull(int field);
    bool reset (const QString& query);
    int size();
    int numRowsAffected();
    QVariant lastInsertId() const;
    QSqlRecord record() const;

#if MYSQL_VERSION_ID >= 40108
    bool prepare(const QString& stmt);
    bool exec();
#endif
private:
    QMYSQLEmbeddedResultPrivate* d;
};

class Q_EXPORT_SQLDRIVER_MYSQL QMYSQLEmbeddedDriver : public QSqlDriver
{
    Q_OBJECT
    friend class QMYSQLEmbeddedResult;
public:
    explicit QMYSQLEmbeddedDriver(QObject *parent=0);
    explicit QMYSQLEmbeddedDriver(MYSQL *con, QObject * parent=0);
    ~QMYSQLEmbeddedDriver();
    bool hasFeature(DriverFeature f) const;
    bool open(const QString & db,
               const QString & user,
               const QString & password,
               const QString & host,
               int port,
               const QString& connOpts);
    void close();
    QSqlResult *createResult() const;
    QStringList tables(QSql::TableType) const;
    QSqlIndex primaryIndex(const QString& tablename) const;
    QSqlRecord record(const QString& tablename) const;
    QString formatValue(const QSqlField &field,
                                     bool trimStrings) const;
    QVariant handle() const;

protected:
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
private:
    void init();
    void qServerInit();
    QMYSQLEmbeddedDriverPrivate* d;
};

QT_END_HEADER

#endif // QSQL_MYSQLEMBEDDED_H
