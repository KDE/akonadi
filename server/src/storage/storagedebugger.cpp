/*
 * Copyright (C) 2013  Daniel Vrátil <dvratil@redhat.com>
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

#include "storagedebugger.h"
#include "storagedebuggeradaptor.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMetaType>

Akonadi::Server::StorageDebugger *Akonadi::Server::StorageDebugger::mSelf = 0;
QMutex Akonadi::Server::StorageDebugger::mMutex;

using namespace Akonadi::Server;

Q_DECLARE_METATYPE( QList< QList<QVariant> > )
Q_DECLARE_METATYPE( DbConnection )
Q_DECLARE_METATYPE( QVector<DbConnection> )

QDBusArgument &operator<<( QDBusArgument &arg, const DbConnection &con )
{
    arg.beginStructure();
    arg << con.id
        << con.name
        << con.start
        << con.transactionStart;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>( const QDBusArgument &arg, DbConnection &con )
{
    arg.beginStructure();
    arg >> con.id
        >> con.name
        >> con.start
        >> con.transactionStart;
    arg.endStructure();
    return arg;
}

namespace {

QVector<DbConnection>::Iterator findConnection( QVector<DbConnection> &vec, qint64 id )
{
    return std::find_if(vec.begin(), vec.end(),
                         [id](const DbConnection &con) {
                             return con.id == id;
                         });
}

}

StorageDebugger *StorageDebugger::instance()
{
  mMutex.lock();
  if ( mSelf == 0 ) {
    mSelf = new StorageDebugger();
  }
  mMutex.unlock();

  return mSelf;
}

StorageDebugger::StorageDebugger()
  : mEnabled( false )
  , mSequence( 0 )
{
  qDBusRegisterMetaType<QList< QList<QVariant> > >();
  qDBusRegisterMetaType<DbConnection>();
  qDBusRegisterMetaType<QVector<DbConnection>>();
  new StorageDebuggerAdaptor( this );
  QDBusConnection::sessionBus().registerObject( QLatin1String("/storageDebug"),
                                                this, QDBusConnection::ExportAdaptors );
}

StorageDebugger::~StorageDebugger()
{
}

void StorageDebugger::enableSQLDebugging( bool enable )
{
  mEnabled = enable;
}

void StorageDebugger::addConnection(qint64 id, const QString &name)
{
    QMutexLocker locker(&mMutex);
    const qint64 time = QDateTime::currentMSecsSinceEpoch();
    mConnections.push_back({ id, name, time, 0ll });

    if (mEnabled) {
        Q_EMIT connectionOpened(id, name, time);
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

void StorageDebugger::addTransaction(qint64 connectionId, uint duration,
                                     const QString &error)
{
    QMutexLocker locker(&mMutex);
    auto con = findConnection(mConnections, connectionId);
    if (con == mConnections.end()) {
        return;
    }
    con->transactionStart = QDateTime::currentMSecsSinceEpoch();

    if (mEnabled) {
        Q_EMIT transactionStarted(connectionId, con->transactionStart, duration, error);
    }

}

void StorageDebugger::removeTransaction(qint64 connectionId, bool commit,
                                        uint duration, const QString &error)
{
    QMutexLocker locker(&mMutex);
    auto con = findConnection(mConnections, connectionId);
    if (con == mConnections.end()) {
        return;
    }
    con->transactionStart = 0;

    if (mEnabled) {
        Q_EMIT transactionFinished(connectionId, commit, QDateTime::currentMSecsSinceEpoch(), duration, error);
    }
}

QVector<DbConnection> StorageDebugger::connections() const
{
    QMutexLocker locker(&mMutex);
    return mConnections;
}

void StorageDebugger::queryExecuted(qint64 connectionId, const QSqlQuery &query, int duration)
{
    const qint64 seq = mSequence.fetchAndAddOrdered(1);
    if (!mEnabled) {
        return;
    }

    if (query.lastError().isValid()) {
        Q_EMIT queryExecuted(seq, connectionId, QDateTime::currentMSecsSinceEpoch(),
                             duration, query.executedQuery(), query.boundValues(), 0,
                             QList< QList<QVariant> >(), query.lastError().text());
        return;
    }

    QSqlQuery q(query);
    QList< QVariantList > result;

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

    int querySize;
    if (query.isSelect()) {
        querySize = query.size();
    } else {
        querySize = query.numRowsAffected();
    }

    Q_EMIT queryExecuted(seq, connectionId, QDateTime::currentMSecsSinceEpoch(),
                         duration, query.executedQuery(), query.boundValues(),
                         querySize, result, QString());

    // Reset the query
    q.seek(-1, false);
}
