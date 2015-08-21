/*
 * Copyright 2015  Daniel Vratil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#ifndef EXTERNALPARTSTORAGE_P_H
#define EXTERNALPARTSTORAGE_P_H

#include "akonadiprivate_export.h"

#include <QtCore/QVector>
#include <QtCore/QMutex>
#include <QtCore/QHash>

class QString;
class QByteArray;
class QThread;

namespace Akonadi {

class AKONADIPRIVATE_EXPORT ExternalPartStorageTransaction
{
public:
    explicit ExternalPartStorageTransaction();
    ~ExternalPartStorageTransaction();

    bool commit();
    bool rollback();
};

/**
 * Provides access to external payload part file storage
 *
 * Use ExternalPartStorageTransaction to delay deletion of part files until
 * commit. Files created during the transaction will be deleted when transaction
 * is rolled back to keep the storage clean.
 */
class AKONADIPRIVATE_EXPORT ExternalPartStorage
{
public:
    static ExternalPartStorage *self();

    static QString resolveAbsolutePath(const QByteArray &filename, bool *exists = Q_NULLPTR, bool legacyFallback = true);
    static QString resolveAbsolutePath(const QString &filename, bool *exists = Q_NULLPTR, bool legacyFallback = true);
    static QByteArray updateFileNameRevision(const QByteArray &filename);
    static QByteArray nameForPartId(qint64 partId);
    static QString akonadiStoragePath();

    bool updatePartFile(const QByteArray &newData, const QByteArray &partFile, QByteArray &newPartFile);
    bool createPartFile(const QByteArray &newData, qint64 partId, QByteArray &partFileName);
    bool removePartFile(const QString &partFile);

    bool inTransaction() const;

private:
    friend class ExternalPartStorageTransaction;

    struct Operation
    {
        enum Type {
            Create,
            Delete
            // We never update files, we always create a new one with increased
            // revision number, hence no "Update"
        };

        Type type;
        QString filename;
    };

    ExternalPartStorage();

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    bool replayTransaction(const QVector<Operation> &trx, bool commit);
    void addToTransaction(const QVector<Operation> &ops);

    static ExternalPartStorage *sInstance;

    mutable QMutex mTransactionLock;
    QHash<QThread*, QVector<Operation>> mTransactions;
};

}

#endif // EXTERNALPARTSTORAGE_P_H
