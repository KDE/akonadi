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

#include "storagedebugger.h"
#include "storagedebuggeradaptor.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>

#include <QDBusConnection>
#include <QDBusMetaType>

Akonadi::Server::StorageDebugger *Akonadi::Server::StorageDebugger::mSelf = 0;
QMutex Akonadi::Server::StorageDebugger::mMutex;

using namespace Akonadi::Server;

Q_DECLARE_METATYPE(QList< QList<QVariant> >)

StorageDebugger *StorageDebugger::instance()
{
    mMutex.lock();
    if (mSelf == 0) {
        mSelf = new StorageDebugger();
    }
    mMutex.unlock();

    return mSelf;
}

StorageDebugger::StorageDebugger()
    : mFile(0)
    , mEnabled(false)
    , mSequence(0)
{
    qDBusRegisterMetaType<QList< QList<QVariant> > >();
    new StorageDebuggerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/storageDebug"),
                                                 this, QDBusConnection::ExportAdaptors);
}

StorageDebugger::~StorageDebugger()
{
    if (mFile) {
        mFile->close();
        delete mFile;
    }
}

void StorageDebugger::enableSQLDebugging(bool enable)
{
    mEnabled = enable;
}

void StorageDebugger::writeToFile( const QString &file )
{
    delete mFile;
    mFile = new QFile(file);
    mFile->open(QIODevice::WriteOnly);
}

void StorageDebugger::queryExecuted(const QSqlQuery &query, int duration)
{
    const qint64 seq = mSequence.fetchAndAddOrdered(1);

    if (!mEnabled) {
        return;
    }

    if (query.lastError().isValid()) {
        Q_EMIT queryExecuted(seq, duration, query.executedQuery(), query.boundValues(),
                             0, QList< QList<QVariant> >(), query.lastError().text());
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

    Q_EMIT queryExecuted(seq, duration, query.executedQuery(),
                         query.boundValues(), querySize, result, QString());

    if (mFile && mFile->isOpen()) {
        QTextStream out(mFile);
        out << query.executedQuery() << " " << duration << "ms " << "\n";
    }

    // Reset the query
    q.seek(-1, false);
}
