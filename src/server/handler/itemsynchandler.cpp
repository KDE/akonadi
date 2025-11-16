/***************************************************************************
 *   SPDX-FileCopyrightText: 2020  Daniel Vrátil <dvratil@kde.org>         *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "itemsynchandler.h"
#include "itemcreatehandler.h"
#include "itemdeletehandler.h"
#include "storage/transaction.h"
#include "storage/querybuilder.h"
#include "storage/selectquerybuilder.h"
#include "storage/datastore.h"
#include "akonadiserver_debug.h"

#include <optional>

using namespace Akonadi::Server;

namespace Akonadi::Server
{

class ItemSyncer
{
public:
    ItemSyncer(ItemSyncHandler *handler, AkonadiServer &akonadi, Connection *connection)
        : mBaseHandler(handler)
        , mAkonadi(akonadi)
        , mConnection(connection)
    {
    }

    virtual ~ItemSyncer() = default;

    virtual bool run(const Protocol::CommandPtr &cmd) = 0;

    template<typename Resp>
    bool failureResponse(const QString &msg)
    {
        auto response = QSharedPointer<Resp>::create();
        response->setError(1, msg);

        try {
            connection()->sendResponse(mTag, response);
        } catch (const ProtocolException &e) {
            qCWarning(AKONADISERVER_LOG) << "ItemSync failed to send error response:" << e.what();
            throw;
        }

        return false;
    }

protected:
    AkonadiServer &akonadi() { return mAkonadi; }
    Connection *connection() { return mConnection; }
    ItemSyncHandler *baseHandler() { return mBaseHandler; }

    virtual bool mergeItem(const Protocol::CommandPtr &cmd)
    {
        ensureTransaction();

        ItemCreateHandler handler(akonadi());
        handler.setCommand(cmd);
        handler.setConnection(connection());
        handler.setTag(baseHandler()->nextTag());

        if (!handler.parseStream()) {
            return false;
        }
        ++mTransactionSize;

        return commitTransactionIfTooLarge();
    }

    virtual bool finalizeSync(const Protocol::CommandPtr &cmd)
    {
        const auto &syncCmd = Protocol::cmdCast<Protocol::EndItemSyncCommand>(cmd);
        if (syncCmd.commit()) {
            if (!commitTransaction()) {
                return false;
            }
        } else {
            rollbackTransaction();
        }

        connection()->sendResponse(baseHandler()->nextTag(), Protocol::EndItemSyncResponsePtr::create());
        return true;
    }

    void ensureTransaction()
    {
        if (!mTransaction.has_value()) {
            mTransaction.emplace(connection()->storageBackend(), QStringLiteral("ItemSyncer"));
            mTransactionSize = 0;
        }
    }

    bool commitTransactionIfTooLarge()
    {
        if (mTransactionSize > mMaxTransactionSize) {
            return commitTransaction();
        }

        return true;
    }

    bool commitTransaction()
    {
        if (!mTransaction.has_value()) {
            return true;
        }

        const bool success = mTransaction->commit();
        mTransaction.reset();
        mTransactionSize = 0;
        return success;
    }

    void rollbackTransaction()
    {
        mTransaction.reset();
        mTransactionSize = 0;
    }

    bool prepareCollection(const Protocol::CommandPtr &cmd)
    {
        const auto beginCmd = Protocol::cmdCast<Protocol::BeginItemSyncCommand>(cmd);

        mCollection = Collection::retrieveById(beginCmd.collectionId());
        if (!mCollection.isValid()) {
            return failureResponse<Protocol::BeginItemSyncResponse>(QStringLiteral("ItemSync failed: invalid collection ID %1")
                    .arg(beginCmd.collectionId()));
        }

        return true;
    }

private:
    ItemSyncHandler *mBaseHandler;
    AkonadiServer &mAkonadi;
    Connection *mConnection;

protected:
    Collection mCollection;
    std::optional<Transaction> mTransaction;
    int mTransactionSize = 0;
    constexpr static int mMaxTransactionSize = 100;
    quint64 mTag = 0;

};

class IncrementalItemSyncer final : public ItemSyncer
{
public:
    using ItemSyncer::ItemSyncer;

    bool run(const Protocol::CommandPtr &cmd) override
    {
        if (!prepareCollection(cmd)) {
            return false;
        }

        qCDebug(AKONADISERVER_LOG) << "IncrementalItemSyncer for collection" << mCollection.id() << "running.";

        // OK, we are ready now
        connection()->sendResponse(Protocol::BeginItemSyncResponse{});

        Q_FOREVER {
            Protocol::CommandPtr cmd;
            while (!cmd) {
                try {
                    cmd = connection()->readCommand();
                } catch (const ProtocolTimeoutException &e) {
                    continue;
                }
            }

            switch (cmd->type()) {
            case Protocol::Command::CreateItem:
                if (!mergeItem(cmd)) {
                    // The failure was reported to the client, now wait for EndItemSync command.
                }
                break;
            case Protocol::Command::DeleteItems:
                if (!deleteItems(cmd)) {
                    // The failure was reported to the client, now wait for EndItemSync command.
                }
                break;
            case Protocol::Command::EndItemSync:
                qCDebug(AKONADISERVER_LOG) << "Finalizing item sync for collection" << mCollection.id();
                return finalizeSync(cmd);
            default:
                qCWarning(AKONADISERVER_LOG) << "Invalid command" << cmd->type() << "received during incremental item sync of collection" << mCollection.id();
                throw HandlerException("Invalid command during received by IncrementalItemSyncer");
            }
        }
    }

private:
    bool deleteItems(const Protocol::CommandPtr &cmd)
    {
        ensureTransaction();

        ItemDeleteHandler handler(akonadi());
        handler.setCommand(cmd);
        handler.setConnection(connection());
        handler.setTag(baseHandler()->nextTag());

        if (!handler.parseStream()) {
            return false;
        }
        ++mTransactionSize;

        return commitTransactionIfTooLarge();
    }
};

class FullItemSyncer final : public ItemSyncer
{
public:
    using ItemSyncer::ItemSyncer;

    bool run(const Protocol::CommandPtr &beginCmd) override
    {
        if (!prepareCollection(beginCmd)) {
            return false;
        }

        qCDebug(AKONADISERVER_LOG) << "FullItemSyncer for collection" << mCollection.id() << "running.";

        if (!prepareLocalItems()) {
            return false;
        }

        // OK, we are ready now
        connection()->sendResponse(Protocol::BeginItemSyncResponse{});

        Q_FOREVER {
            Protocol::CommandPtr cmd;
            while (!cmd) {
                try {
                    cmd = connection()->readCommand();
                } catch (const ProtocolTimeoutException &e) {
                    continue;
                }
            }

            switch (cmd->type()) {
            case Protocol::Command::CreateItem:
                if (!mergeItem(cmd)) {
                    // The failure was reported to the client, now wait for EndItemSync command.
                }
                break;
            case Protocol::Command::EndItemSync:
                qCDebug(AKONADISERVER_LOG) << "Finalizing item sync for collection" << mCollection.id();
                return finalizeSync(cmd);
            default:
                qCWarning(AKONADISERVER_LOG) << "Invalid command" << cmd->type() << "received during full item sync of collection" << mCollection.id();
                throw HandlerException("Invalid command received during ItemSync.");
            }
        }

        return true;
    }

protected:
    bool mergeItem(const Protocol::CommandPtr &cmd) override
    {
        const auto &mergeCmd = Protocol::cmdCast<Protocol::CreateItemCommand>(cmd);
        mLocalRids.remove(mergeCmd.remoteId());

        return ItemSyncer::mergeItem(cmd);
    }

    bool finalizeSync(const Protocol::CommandPtr &cmd) override
    {
        const auto &endCmd = Protocol::cmdCast<Protocol::EndItemSyncCommand>(cmd);
        if (endCmd.commit()) {
            if (!commitTransaction()) {
                return false;
            }

            if (!mLocalRids.isEmpty()) {
                Transaction transaction(connection()->storageBackend(), QStringLiteral("ItemSyncFinalize"));
                // Whatever is left in mLocalRids should be removed now
                SelectQueryBuilder<PimItem> qb;
                Query::Condition cond;
                cond.addValueCondition(PimItem::collectionIdColumn(), Query::Equals, mCollection.id());
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                cond.addValueCondition(PimItem::remoteIdColumn(), Query::In, mLocalRids.toList());
#else
                cond.addValueCondition(PimItem::remoteIdColumn(), Query::In, QStringList(mLocalRids.begin(), mLocalRids.end()));
#endif
                qb.addCondition(cond);
                if (!qb.exec()) {
                    return failureResponse<Protocol::EndItemSyncResponse>(QStringLiteral("ItemSync failed to query local items to delete after sync."));
                }
                connection()->storageBackend()->cleanupPimItems(qb.result());

                transaction.commit();
            }
        }

        return ItemSyncer::finalizeSync(cmd);
    }

private:
    bool prepareLocalItems()
    {
        QueryBuilder qb(PimItem::tableName());
        qb.addColumn(PimItem::remoteIdColumn());
        Query::Condition cond;
        cond.addValueCondition(PimItem::remoteIdColumn(), Query::IsNot, QVariant{});
        cond.addValueCondition(PimItem::collectionIdColumn(), Query::Equals, mCollection.id());
        qb.addCondition(cond);

        if (!qb.exec()) {
            return failureResponse<Protocol::BeginItemSyncResponse>(QStringLiteral("ItemSync failed to query local items"));
        }

        auto &query = qb.query();
        while (query.next()) {
            mLocalRids.insert(query.value(0).toString());
        }

        qCDebug(AKONADISERVER_LOG) << "Local listing returned" << mLocalRids.size() << "existing items";

        return true;
    }

private:
    QSet<QString> mLocalRids;
};

} // namespace Akonadi::Server


ItemSyncHandler::ItemSyncHandler(AkonadiServer &server):
    Handler(server)
{}

std::unique_ptr<ItemSyncer> ItemSyncHandler::createSyncer(ItemSyncHandler *handler, bool incremental) const
{
    if (incremental) {
        return std::make_unique<IncrementalItemSyncer>(handler, akonadi(), connection());
    } else {
        return std::make_unique<FullItemSyncer>(handler, akonadi(), connection());
    }
}

bool ItemSyncHandler::parseStream()
{
    auto begin = Protocol::cmdCast<Protocol::BeginItemSyncCommand>(command());

    auto syncer = createSyncer(this, begin.incremental());
    return syncer->run(command());
}

quint64 ItemSyncHandler::nextTag()
{
    auto t = tag();
    setTag(++t);

    return t;
}
