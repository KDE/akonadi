/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

// krazy:excludeall=dpointer

#pragma once

#include "akonadiprivate_export.h"

#include <QHash>
#include <QMutex>
#include <QVector>

class QString;
class QByteArray;
class QThread;

namespace Akonadi
{
class AKONADIPRIVATE_EXPORT ExternalPartStorageTransaction
{
public:
    explicit ExternalPartStorageTransaction();
    ~ExternalPartStorageTransaction();

    bool commit();
    bool rollback();

private:
    Q_DISABLE_COPY(ExternalPartStorageTransaction)
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

    static QString resolveAbsolutePath(const QByteArray &filename, bool *exists = nullptr, bool legacyFallback = true);
    static QString resolveAbsolutePath(const QString &filename, bool *exists = nullptr, bool legacyFallback = true);
    static QByteArray updateFileNameRevision(const QByteArray &filename);
    static QByteArray nameForPartId(qint64 partId);
    static QString akonadiStoragePath();

    bool updatePartFile(const QByteArray &newData, const QByteArray &partFile, QByteArray &newPartFile);
    bool createPartFile(const QByteArray &newData, qint64 partId, QByteArray &partFileName);
    bool removePartFile(const QString &partFile);

    bool inTransaction() const;

private:
    friend class ExternalPartStorageTransaction;

    struct Operation {
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

    mutable QMutex mTransactionLock;
    QHash<QThread *, QVector<Operation>> mTransactions;
};

}
