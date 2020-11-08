/*
    SPDX-FileCopyrightText: 2009 Bertjan Broeksema <broeksema@kde.org>
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "sqlite_blocking.h"

#include <sqlite3.h>

#include <QMutex>
#include <QWaitCondition>
#include "qdebug.h"
#include <QStringBuilder>
#include <QThread>

QString debugString()
{
    return QString(QLatin1String("[QSQLITE3: ") + QString::number(quint64(QThread::currentThreadId())) + QLatin1String("] "));
}

/* Based on example in http://www.sqlite.org/unlock_notify.html */

struct UnlockNotification {
    bool fired;
    QWaitCondition cond;
    QMutex mutex;
};

static void qSqlite3UnlockNotifyCb(void **apArg, int nArg)
{
    for (int i = 0; i < nArg; ++i) {
        auto *ntf = static_cast<UnlockNotification *>(apArg[i]);
        ntf->mutex.lock();
        ntf->fired = true;
        ntf->cond.wakeOne();
        ntf->mutex.unlock();
    }
}

static int qSqlite3WaitForUnlockNotify(sqlite3 *db)
{
    int rc;
    UnlockNotification un;
    un.fired = false;

    rc = sqlite3_unlock_notify(db, qSqlite3UnlockNotifyCb, (void *)&un);
    Q_ASSERT(rc == SQLITE_LOCKED || rc == SQLITE_OK);

    if (rc == SQLITE_OK) {
        un.mutex.lock();
        if (!un.fired) {
            un.cond.wait(&un.mutex);
        }
        un.mutex.unlock();
    }

    return rc;
}

int sqlite3_blocking_step(sqlite3_stmt *pStmt)
{
    int rc;
    while (SQLITE_LOCKED_SHAREDCACHE == (rc = sqlite3_step(pStmt))) {
        //qDebug() << debugString() << "sqlite3_blocking_step: Waiting..."; QTime now; now.start();
        rc = qSqlite3WaitForUnlockNotify(sqlite3_db_handle(pStmt));
        //qDebug() << debugString() << "sqlite3_blocking_step: Waited for " << now.elapsed() << "ms";
        if (rc != SQLITE_OK) {
            break;
        }
        sqlite3_reset(pStmt);
    }

    return rc;
}

int sqlite3_blocking_prepare16_v2(sqlite3 *db, const void *zSql, int nSql,
                                  sqlite3_stmt **ppStmt, const void **pzTail)
{
    int rc;
    while (SQLITE_LOCKED_SHAREDCACHE == (rc = sqlite3_prepare16_v2(db, zSql, nSql, ppStmt, pzTail))) {
        //qDebug() << debugString() << "sqlite3_blocking_prepare16_v2: Waiting..."; QTime now; now.start();
        rc = qSqlite3WaitForUnlockNotify(db);
        //qDebug() << debugString() << "sqlite3_blocking_prepare16_v2: Waited for " << now.elapsed() << "ms";
        if (rc != SQLITE_OK) {
            break;
        }
    }

    return rc;
}
