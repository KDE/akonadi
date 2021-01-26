/*
 * SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "storagedebugger.h"
#include "storagedebuggeradaptor.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include <QDBusConnection>

Akonadi::Server::StorageDebugger *Akonadi::Server::StorageDebugger::mSelf = nullptr;
QMutex Akonadi::Server::StorageDebugger::mMutex;

using namespace Akonadi::Server;

Q_DECLARE_METATYPE(QList<QList<QVariant>>)
Q_DECLARE_METATYPE(DbConnection)
Q_DECLARE_METATYPE(QVector<DbConnection>)

QDBusArgument &operator<<(QDBusArgument &arg, const DbConnection &con)
{
    arg.beginStructure();
    arg << con.id << con.name << con.start << con.trxName << con.transactionStart;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, DbConnection &con)
{
    arg.beginStructure();
    arg >> con.id >> con.name >> con.start >> con.trxName >> con.transactionStart;
    arg.endStructure();
    return arg;
}

namespace
{
QVector<DbConnection>::Iterator findConnection(QVector<DbConnection> &vec, qint64 id)
{
    return std::find_if(vec.begin(), vec.end(), [id](const DbConnection &con) {
        return con.id == id;
    });
}

} // namespace

StorageDebugger *StorageDebugger::instance()
{
    mMutex.lock();
    if (mSelf == nullptr) {
        mSelf = new StorageDebugger();
    }
    mMutex.unlock();

    return mSelf;
}

StorageDebugger::StorageDebugger()
{
    qDBusRegisterMetaType<QList<QList<QVariant>>>();
    qDBusRegisterMetaType<DbConnection>();
    qDBusRegisterMetaType<QVector<DbConnection>>();
    new StorageDebuggerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/storageDebug"), this, QDBusConnection::ExportAdaptors);
}

StorageDebugger::~StorageDebugger() = default;

void StorageDebugger::enableSQLDebugging(bool enable)
{
    mEnabled = enable;
}

void StorageDebugger::writeToFile(const QString &file)
{
    mFile.reset(new QFile(file));
    mFile->open(QIODevice::WriteOnly);
}

void StorageDebugger::addConnection(qint64 id, const QString &name)
{
    QMutexLocker locker(&mMutex);
    const qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    mConnections.push_back({id, name, timestamp, QString(), 0LL});
    if (mEnabled) {
        Q_EMIT connectionOpened(id, timestamp, name);
    }
}

void StorageDebugger::removeConnection(qint64 id)
{
    QMutexLocker locker(&mMutex);
    auto con = findConnection(mConnections, id);
    if (con == mConnections.end()) {
        return;
    }
    mConnections.erase(con);

    if (mEnabled) {
        Q_EMIT connectionClosed(id, QDateTime::currentMSecsSinceEpoch());
    }
}

void StorageDebugger::changeConnection(qint64 id, const QString &name)
{
    QMutexLocker locker(&mMutex);
    auto con = findConnection(mConnections, id);
    if (con == mConnections.end()) {
        return;
    }
    con->name = name;

    if (mEnabled) {
        Q_EMIT connectionChanged(id, name);
    }
}

void StorageDebugger::addTransaction(qint64 connectionId, const QString &name, uint duration, const QString &error)
{
    QMutexLocker locker(&mMutex);
    auto con = findConnection(mConnections, connectionId);
    if (con == mConnections.end()) {
        return;
    }
    con->trxName = name;
    con->transactionStart = QDateTime::currentMSecsSinceEpoch();

    if (mEnabled) {
        Q_EMIT transactionStarted(connectionId, name, con->transactionStart, duration, error);
    }
}

void StorageDebugger::removeTransaction(qint64 connectionId, bool commit, uint duration, const QString &error)
{
    QMutexLocker locker(&mMutex);
    auto con = findConnection(mConnections, connectionId);
    if (con == mConnections.end()) {
        return;
    }
    con->trxName.clear();
    con->transactionStart = 0;

    if (mEnabled) {
        Q_EMIT transactionFinished(connectionId, commit, QDateTime::currentMSecsSinceEpoch(), duration, error);
    }
}

QVector<DbConnection> StorageDebugger::connections() const
{
    return mConnections;
}

void StorageDebugger::queryExecuted(qint64 connectionId, const QSqlQuery &query, int duration)
{
    if (!mEnabled) {
        return;
    }

    const qint64 seq = ++mSequence;
    if (query.lastError().isValid()) {
        Q_EMIT queryExecuted(seq,
                             connectionId,
                             QDateTime::currentMSecsSinceEpoch(),
                             duration,
                             query.executedQuery(),
                             query.boundValues(),
                             0,
                             QList<QList<QVariant>>(),
                             query.lastError().text());
        return;
    }

    QSqlQuery q(query);
    QList<QVariantList> result;

    if (q.first()) {
        const QSqlRecord record = q.record();
        QVariantList row;
        const int numRecords = record.count();
        row.reserve(numRecords);
        for (int i = 0; i < numRecords; ++i) {
            row << record.fieldName(i);
        }
        result << row;

        int cnt = 0;
        do {
            const QSqlRecord record = q.record();
            QVariantList row;
            const int numRecords = record.count();
            row.reserve(numRecords);
            for (int i = 0; i < numRecords; ++i) {
                row << record.value(i);
            }
            result << row;
        } while (q.next() && ++cnt < 1000);
    }

    const int querySize = query.isSelect() ? query.size() : query.numRowsAffected();
    Q_EMIT queryExecuted(seq,
                         connectionId,
                         QDateTime::currentMSecsSinceEpoch(),
                         duration,
                         query.executedQuery(),
                         query.boundValues(),
                         querySize,
                         result,
                         QString());

    if (mFile && mFile->isOpen()) {
        QTextStream out(mFile.get());
        out << query.executedQuery() << " " << duration << "ms\n";
    }

    // Reset the query
    q.seek(-1, false);
}
