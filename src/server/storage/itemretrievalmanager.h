/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akthread.h"
#include "itemretrievalrequest.h"
#include "itemretriever.h"
#include <shared/akstd.h>

#include <QHash>
class QObject;
#include <QReadWriteLock>
#include <QWaitCondition>

#include <unordered_map>

class OrgFreedesktopAkonadiResourceInterface;

namespace Akonadi
{
namespace Server
{
class Collection;
class ItemRetrievalJob;
class AbstractItemRetrievalJob;

class AbstractItemRetrievalJobFactory
{
public:
    virtual ~AbstractItemRetrievalJobFactory() = default;

    virtual AbstractItemRetrievalJob *retrievalJob(ItemRetrievalRequest request, QObject *parent) = 0;

protected:
    explicit AbstractItemRetrievalJobFactory() = default;

private:
    Q_DISABLE_COPY_MOVE(AbstractItemRetrievalJobFactory)
};

/** Manages and processes item retrieval requests. */
class ItemRetrievalManager : public AkThread
{
    Q_OBJECT
protected:
    /**
     * Use AkThread::create() to create and start a new ItemRetrievalManager thread.
     */
    explicit ItemRetrievalManager(QObject *parent = nullptr);
    explicit ItemRetrievalManager(std::unique_ptr<AbstractItemRetrievalJobFactory> factory, QObject *parent = nullptr);

public:
    ~ItemRetrievalManager() override;

    /**
     * Added for convenience. ItemRetrievalManager takes ownership over the
     * pointer and deletes it when the request is processed.
     */
    virtual void requestItemDelivery(ItemRetrievalRequest request);

    void triggerCollectionSync(const QString &resource, qint64 colId);
    void triggerCollectionTreeSync(const QString &resource);

Q_SIGNALS:
    void requestFinished(const Akonadi::Server::ItemRetrievalResult &result);
    void requestAdded();

private:
    OrgFreedesktopAkonadiResourceInterface *resourceInterface(const QString &id);
    QVector<AbstractItemRetrievalJob *> scheduleJobsForIdleResourcesLocked();

private Q_SLOTS:
    void init() override;

    void serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);
    void processRequest();
    void retrievalJobFinished(Akonadi::Server::AbstractItemRetrievalJob *job);

protected:
    std::unique_ptr<AbstractItemRetrievalJobFactory> mJobFactory;

    /// Protects mPendingRequests and every Request object posted to it
    QReadWriteLock mLock;
    /// Used to let requesting threads wait until the request has been processed
    QWaitCondition mWaitCondition;

    /// Pending requests queues, one per resource
    std::unordered_map<QString, std::list<ItemRetrievalRequest>> mPendingRequests;
    /// Currently running jobs, one per resource
    QHash<QString, AbstractItemRetrievalJob *> mCurrentJobs;

    // resource dbus interface cache
    std::unordered_map<QString, std::unique_ptr<OrgFreedesktopAkonadiResourceInterface>> mResourceInterfaces;
};

} // namespace Server
} // namespace Akonadi
