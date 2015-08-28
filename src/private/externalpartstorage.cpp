/*
 * Copyright (C) 2015  Daniel Vrátil <dvratil@redhat.com>
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

#include "externalpartstorage_p.h"
#include "standarddirs_p.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMutexLocker>
#include <QtCore/QThread>
#include <QtCore/QDebug>

#include <mutex>

using namespace Akonadi;

ExternalPartStorage *ExternalPartStorage::sInstance = Q_NULLPTR;
std::once_flag storageOnce;


ExternalPartStorageTransaction::ExternalPartStorageTransaction()
{
    ExternalPartStorage::self()->beginTransaction();
}

ExternalPartStorageTransaction::~ExternalPartStorageTransaction()
{
    if (ExternalPartStorage::self()->inTransaction()) {
        rollback();
    }
}

bool ExternalPartStorageTransaction::commit()
{
    return ExternalPartStorage::self()->commitTransaction();
}

bool ExternalPartStorageTransaction::rollback()
{
    return ExternalPartStorage::self()->rollbackTransaction();
}




ExternalPartStorage::ExternalPartStorage()
{
}

ExternalPartStorage *ExternalPartStorage::self()
{
    std::call_once(storageOnce, []() { sInstance = new ExternalPartStorage(); });
    return sInstance;
}

QString ExternalPartStorage::resolveAbsolutePath(const QByteArray &filename, bool *exists, bool legacyFallback)
{
    return resolveAbsolutePath(QString::fromLocal8Bit(filename), exists, legacyFallback);
}

QString ExternalPartStorage::resolveAbsolutePath(const QString &filename, bool *exists, bool legacyFallback)
{
    if (exists) {
        *exists = false;
    }

    QFileInfo finfo(filename);
    if (finfo.isAbsolute()) {
        if (exists && finfo.exists()) {
            *exists = true;
        }
        return filename;
    }

    const QString basePath = StandardDirs::saveDir("data", QStringLiteral("file_db_data"));
    Q_ASSERT(!basePath.isEmpty());

    // Part files are stored in levelled cache. We use modulo 100 of the partID
    // to ensure even distribution of the part files into the subfolders.
    // PartID is encoded in filename as "PARTID_rX".
    const int revPos = filename.indexOf(QLatin1Char('_'));
    const QString path = basePath
                        + QDir::separator()
                            + (revPos > 1 ? filename[revPos - 2] : QLatin1Char('0'))
                            + filename[revPos - 1]
                        + QDir::separator()
                            + filename;
    // If legacy fallback is disabled, return it in any case
    if (!legacyFallback) {
        QFileInfo finfo(path);
        QDir().mkpath(finfo.path());
        return path;
    }

    // ..otherwise return it only if it exists
    if (QFile::exists(path)) {
        if (exists) {
            *exists = true;
        }
        return path;
    }

    // .. and fallback to legacy if it does not, but only when legacy exists
    const QString legacyPath = basePath + QDir::separator() + filename;
    if (QFile::exists(legacyPath)) {
        if (exists) {
            *exists = true;
        }
        return legacyPath;
    } else {
        QFileInfo finfo(path);
        QDir().mkpath(finfo.path());
        // If neither legacy or new path exists, return the new path, so that
        // new items are created in the correct location
        return path;
    }
}

bool ExternalPartStorage::createPartFile(const QByteArray &data, qint64 partId,
                                        QByteArray &partFileName)
{
    bool exists = false;
    partFileName = updateFileNameRevision(QByteArray::number(partId));
    const QString path = resolveAbsolutePath(partFileName, &exists);
    if (exists) {
        qWarning() << "Error: asked to create a part" << partFileName << ", which already exists!";
        return false;
    }

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "Error: failed to open new part file for writing:" << f.errorString();
        return false;
    }
    if (f.write(data) != data.size()) {
        // TODO: Maybe just try again?
        qWarning() << "Error: failed to write all data into the part file";
        return false;
    }
    f.close();

    if (inTransaction()) {
        addToTransaction({ { Operation::Create, path } });
    }
    return true;
}

bool ExternalPartStorage::updatePartFile(const QByteArray &newData, const QByteArray &partFile,
                                        QByteArray &newPartFile)
{
    bool exists = false;
    const QString currentPartPath = resolveAbsolutePath(partFile, &exists);
    if (!exists) {
        qWarning() << "Error: asked to update a non-existent part, aborting update";
        return false;
    }

    newPartFile = updateFileNameRevision(partFile);
    exists = false;
    const QString newPartPath = resolveAbsolutePath(newPartFile, &exists);
    if (exists) {
        qWarning() << "Error: asked to update part" << partFile << ", but" << newPartFile << "already exists, aborting update";
        return false;
    }

    QFile f(newPartPath);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "Error: failed to open new part file for update:" << f.errorString();
        return false;
    }

    if (f.write(newData) != newData.size()) {
        // TODO: Maybe just try again?
        qWarning() << "Error: failed to write all data into the part file";
        return false;
    }
    f.close();

    if (inTransaction()) {
        addToTransaction({ { Operation::Create, newPartPath },
                           { Operation::Delete, currentPartPath } });
    } else {
        if (!QFile::remove(currentPartPath)) {
            // Not a reason to fail the operation
            qWarning() << "Error: failed to remove old part payload file" << currentPartPath;
        }
    }

    return true;
}

bool ExternalPartStorage::removePartFile(const QString &partFile)
{
    if (inTransaction()) {
        addToTransaction({ { Operation::Delete, partFile } });
    } else {
        if (!QFile::remove(partFile)) {
            // Not a reason to fail the operation
            qWarning() << "Error: failed to remove part file" << partFile;
        }
    }

    return true;
}

QByteArray ExternalPartStorage::updateFileNameRevision(const QByteArray &filename)
{
    const int revIndex = filename.indexOf("_r");
    if (revIndex > -1) {
        QByteArray rev = filename.mid(revIndex + 2);
        int r = rev.toInt();
        r++;
        rev = QByteArray::number(r);
        return filename.left(revIndex + 2) + rev;
    }

    return filename + "_r0";
}

QByteArray ExternalPartStorage::nameForPartId(qint64 partId)
{
    return QByteArray::number(partId) + "_r0";
}

bool ExternalPartStorage::beginTransaction()
{
    QMutexLocker locker(&mTransactionLock);
    if (mTransactions.contains(QThread::currentThread())) {
        qDebug() << "Error: there is already a transaction in progress in this thread";
        return false;
    }

    mTransactions.insert(QThread::currentThread(), QVector<Operation>());
    return true;
}

QString ExternalPartStorage::akonadiStoragePath()
{
    return StandardDirs::saveDir("data", QStringLiteral("file_db_data"));
}

bool ExternalPartStorage::commitTransaction()
{
    QMutexLocker locker(&mTransactionLock);
    auto iter = mTransactions.find(QThread::currentThread());
    if (iter == mTransactions.end()) {
        qDebug() << "Commit error: there is no transaction in progress in this thread";
        return false;
    }

    const QVector<Operation> trx = iter.value();
    mTransactions.erase(iter);
    locker.unlock();

    return replayTransaction(trx, true);
}

bool ExternalPartStorage::rollbackTransaction()
{
    QMutexLocker locker(&mTransactionLock);
    auto iter = mTransactions.find(QThread::currentThread());
    if (iter == mTransactions.end()) {
        qDebug() << "Rollback error: there is no transaction in progress in this thread";
        return false;
    }

    const QVector<Operation> trx = iter.value();
    mTransactions.erase(iter);
    locker.unlock();

    return replayTransaction(trx, false);
}

bool ExternalPartStorage::inTransaction() const
{
    QMutexLocker locker(&mTransactionLock);
    return mTransactions.contains(QThread::currentThread());
}

void ExternalPartStorage::addToTransaction(const QVector<Operation> &ops)
{
    QMutexLocker locker(&mTransactionLock);
    auto iter = mTransactions.find(QThread::currentThread());
    Q_ASSERT(iter != mTransactions.end());
    locker.unlock();

    Q_FOREACH (const Operation &op, ops) {
        iter->append(op);
    }
}

bool ExternalPartStorage::replayTransaction(const QVector<Operation> &trx, bool commit)
{
    for (auto iter = trx.constBegin(), end = trx.constEnd(); iter != end; ++iter) {
        const Operation &op = *iter;

        if (op.type == Operation::Create) {
            if (commit) {
                // no-op: we actually created that already in createPart()/updatePart()
            } else {
                if (!QFile::remove(op.filename)) {
                    // We failed to remove the file, but don't abort the rollback.
                    // This is an error, but does not cause data loss.
                    qDebug() << "Warning: failed to remove" << op.filename << "while rolling back a transaction";
                }
            }
        } else if (op.type == Operation::Delete) {
            if (commit) {
                if (!QFile::remove(op.filename)) {
                    // We failed to remove the file, but don't abort the commit.
                    // This is an error, but does not cause data loss.
                    qDebug() << "Warning: failed to remove" << op.filename << "while committing a transaction";
                }
            } else {
                // no-op: we did not actually delete the file yet
            }
        } else {
            Q_UNREACHABLE();
        }
    }

    return true;
}
