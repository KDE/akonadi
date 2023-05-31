/*
 * SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once

#include <QFile>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QVariant>

#include <atomic>
#include <memory>

class QSqlQuery;
class QDBusArgument;

struct DbConnection {
    qint64 id;
    QString name;
    qint64 start;
    QString trxName;
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

    ~StorageDebugger() override;

    void addConnection(qint64 id, const QString &name);
    void removeConnection(qint64 id);
    void changeConnection(qint64 id, const QString &name);
    void addTransaction(qint64 connectionId, const QString &name, uint duration, const QString &error);
    void removeTransaction(qint64 connectionId, bool commit, uint duration, const QString &error);

    void enableSQLDebugging(bool enable);
    inline bool isSQLDebuggingEnabled() const
    {
        return mEnabled;
    }

    void queryExecuted(qint64 connectionId, const QSqlQuery &query, int duration);

    inline void incSequence()
    {
        ++mSequence;
    }

    void writeToFile(const QString &file);

    Q_SCRIPTABLE QList<DbConnection> connections() const;

Q_SIGNALS:
    void connectionOpened(qint64 id, qint64 timestamp, const QString &name);
    void connectionChanged(qint64 id, const QString &name);
    void connectionClosed(qint64 id, qint64 timestamp);

    void transactionStarted(qint64 connectionId, const QString &name, qint64 timestamp, uint duration, const QString &error);
    void transactionFinished(qint64 connectionId, bool commit, qint64 timestamp, uint duration, const QString &error);
    void queryExecuted(double sequence,
                       qint64 connectionId,
                       qint64 timestamp,
                       uint duration,
                       const QString &query,
                       const QVariantList &values,
                       int resultsCount,
                       const QList<QList<QVariant>> &result,
                       const QString &error);

private:
    StorageDebugger();

    static StorageDebugger *mSelf;
    static QMutex mMutex;

    std::unique_ptr<QFile> mFile;

    std::atomic_bool mEnabled = {false};
    std::atomic_int64_t mSequence = {0};
    QList<DbConnection> mConnections;
};

} // namespace Server
} // namespace Akonadi
