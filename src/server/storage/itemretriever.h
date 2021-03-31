/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDateTime>
#include <QObject>

#include "../exception.h"
#include "entities.h"

#include <private/imapset_p.h>
#include <private/scope_p.h>

#include <optional>

AKONADI_EXCEPTION_MAKE_INSTANCE(ItemRetrieverException);

namespace Akonadi
{
namespace Server
{
class Connection;
class CommandContext;
class ItemRetrievalManager;
class ItemRetrievalRequest;

/**
  Helper class for retrieving missing items parts from remote resources.

  Stuff in here happens in the calling thread and does not access shared data.

  @todo make usable for Fetch by allowing to share queries
*/
class ItemRetriever : public QObject
{
    Q_OBJECT

public:
    explicit ItemRetriever(ItemRetrievalManager &manager, Connection *connection, const CommandContext &context);

    Connection *connection() const;

    void setRetrieveParts(const QVector<QByteArray> &parts);
    QVector<QByteArray> retrieveParts() const;
    void setRetrieveFullPayload(bool fullPayload);
    void setChangedSince(const QDateTime &changedSince);
    void setItemSet(const ImapSet &set, const Collection &collection = Collection());
    void setItemSet(const ImapSet &set, bool isUid);
    void setItem(Entity::Id id);
    /** Retrieve all items in the given collection. */
    void setCollection(const Collection &collection, bool recursive = true);

    /** Retrieve all items matching the given item scope. */
    void setScope(const Scope &scope);
    Scope scope() const;

    bool exec();

    QByteArray lastError() const;

Q_SIGNALS:
    void itemsRetrieved(const QVector<qint64> &ids);

private:
    QSqlQuery buildQuery() const;

    /**
     * Checks if external files are still present
     * This costs extra, but allows us to automatically recover from something changing the external file storage.
     */
    void verifyCache();

    /// Execute the retrieval
    bool runItemRetrievalRequests(std::list<ItemRetrievalRequest> requests);
    struct PreparedRequests {
        std::list<ItemRetrievalRequest> requests;
        QVector<qint64> readyItems;
    };
    std::optional<PreparedRequests> prepareRequests(QSqlQuery &query, const QByteArrayList &parts);

    Akonadi::ImapSet mItemSet;
    Collection mCollection;
    Scope mScope;
    ItemRetrievalManager &mItemRetrievalManager;
    Connection *mConnection = nullptr;
    const CommandContext &mContext;
    QVector<QByteArray> mParts;
    bool mFullPayload;
    bool mRecursive;
    QDateTime mChangedSince;
    mutable QByteArray mLastError;
    bool mCanceled;
};

} // namespace Server
} // namespace Akonadi

