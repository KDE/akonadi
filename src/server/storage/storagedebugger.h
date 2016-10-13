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

namespace Akonadi {
namespace Server {

class StorageDebugger : public QObject
{
    Q_OBJECT

public:
    static StorageDebugger *instance();

    ~StorageDebugger();

    void enableSQLDebugging(bool enable);
    inline bool isSQLDebuggingEnabled() const
    {
        return mEnabled;
    }

    void queryExecuted(const QSqlQuery &query, int duration);

    void incSequence()
    {
        mSequence.ref();
    }

    void writeToFile(const QString &file);

Q_SIGNALS:
    void queryExecuted(double sequence, uint duration, const QString &query,
                       const QMap<QString, QVariant> &values,
                       int resultsCount,
                       const QList<QList<QVariant> > &result,
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

};

} // namespace Server
} // namespace Akonadi

#endif // STORAGEDEBUGGER_H
