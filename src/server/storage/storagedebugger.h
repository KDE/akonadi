/*
 * Copyright (C) 2013  Daniel Vrátil <dvratil@redhat.com>
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

#ifndef STORAGEDEBUGGER_H
#define STORAGEDEBUGGER_H

#include <QObject>
#include <QMutex>
#include <QMap>
#include <QVariant>
#include <QFile>
#include <QVector>

#ifdef QT5_BUILD
#include <QAtomicInteger>
#else
#include <QAtomicInt>
#endif

#ifdef Q_ATOMC_INT64_IS_SUPPORTED
#include <QAtomicInteger>
#else
#include <QAtomicInt>
#endif

class QSqlQuery;
class QDBusArgument;

struct DbConnection {
    qint64 id;
    QString name;
    qint64 start;
    qint64 transactionStart;
};

QDBusArgument &operator<<(QDBusArgument &arg, const DbConnection &con);
QDBusArgument &operator>>(QDBusArgument &arg, DbConnection &con);

namespace Akonadi
{
namespace Server
{

class StorageDebugger : public QObject
{
    Q_OBJECT

public:
    static StorageDebugger *instance();

    ~StorageDebugger();

    void addConnection(qint64 id, const QString &name);
    void removeConnection(qint64 id);
    void changeConnection(qint64 id, const QString &name);
    void addTransaction(qint64 connectionId, uint duration, const QString &error);
    void removeTransaction(qint64 connectionId, bool commit, uint duration, const QString &error);

    void enableSQLDebugging(bool enable);
    inline bool isSQLDebuggingEnabled() const
    {
        return mEnabled;
    }

    void queryExecuted(qint64 connectionId, const QSqlQuery &query, int duration);

    void incSequence()
    {
        mSequence.ref();
    }

    void writeToFile(const QString &file);

    Q_SCRIPTABLE QVector<DbConnection> connections() const;

Q_SIGNALS:
    void connectionOpened(qint64 id, qint64 timestamp, const QString &name);
    void connectionChanged(qint64 id, const QString &name);
    void connectionClosed(qint64 id, qint64 timestamp);

    void transactionStarted(qint64 connectionId, qint64 timestamp, uint duration, const QString &error);
    void transactionFinished(qint64 connectionId, bool commit, qint64 timestamp, uint duration,
                             const QString &error);

    void queryExecuted(double sequence, qint64 connectionId, qint64 timestamp,
                       uint duration, const QString &query, const QMap<QString, QVariant> &values,
                       int resultsCount, const QList<QList<QVariant> > &result,
                       const QString &error);

private:
    StorageDebugger();

    static StorageDebugger *mSelf;
    static QMutex mMutex;

    QFile *mFile;

    bool mEnabled;
#ifdef Q_ATOMC_INT64_IS_SUPPORTED
    QAtomicInteger<qint64> mSequence;
#else
    QAtomicInt mSequence;
#endif
    QVector<DbConnection> mConnections;

};

} // namespace Server
} // namespace Akonadi

#endif // STORAGEDEBUGGER_H
