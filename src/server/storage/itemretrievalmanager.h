/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AKONADI_ITEMRETRIEVALMANAGER_H
#define AKONADI_ITEMRETRIEVALMANAGER_H

#include "itemretriever.h"
#include "akthread.h"

#include <QHash>
#include <QObject>

class QReadWriteLock;
class QWaitCondition;
class OrgFreedesktopAkonadiResourceInterface;

namespace Akonadi
{
namespace Server
{

class Collection;
class ItemRetrievalJob;
class ItemRetrievalRequest;
class AbstractItemRetrievalJob;

class AbstractItemRetrievalJobFactory
{
public:
    explicit AbstractItemRetrievalJobFactory() {}
    virtual ~AbstractItemRetrievalJobFactory() {}

    virtual AbstractItemRetrievalJob *retrievalJob(ItemRetrievalRequest *request, QObject *parent) = 0;
};

/** Manages and processes item retrieval requests. */
class ItemRetrievalManager : public AkThread
{
    Q_OBJECT
public:
    explicit ItemRetrievalManager(QObject *parent = nullptr);
    explicit ItemRetrievalManager(std::unique_ptr<AbstractItemRetrievalJobFactory> factory, QObject *parent = nullptr);
    ~ItemRetrievalManager() override;

    /**
     * Added for convenience. ItemRetrievalManager takes ownership over the
     * pointer and deletes it when the request is processed.
     */
    virtual void requestItemDelivery(ItemRetrievalRequest *request);

    static ItemRetrievalManager *instance();

Q_SIGNALS:
    void requestFinished(ItemRetrievalRequest *request);
    void requestAdded();

private:
    OrgFreedesktopAkonadiResourceInterface *resourceInterface(const QString &id);

private Q_SLOTS:
    void init() override;

    void serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);
    void processRequest();
    void triggerCollectionSync(const QString &resource, qint64 colId);
    void triggerCollectionTreeSync(const QString &resource);
    void retrievalJobFinished(ItemRetrievalRequest *request, const QString &errorMsg);

protected:
    static ItemRetrievalManager *sInstance;

    std::unique_ptr<AbstractItemRetrievalJobFactory> mJobFactory;

    /// Protects mPendingRequests and every Request object posted to it
    QReadWriteLock *mLock = nullptr;
    /// Used to let requesting threads wait until the request has been processed
    QWaitCondition *mWaitCondition = nullptr;
    /// Pending requests queues, one per resource
    QHash<QString, QList<ItemRetrievalRequest *> > mPendingRequests;
    /// Currently running jobs, one per resource
    QHash<QString, AbstractItemRetrievalJob *> mCurrentJobs;

    // resource dbus interface cache
    QHash<QString, OrgFreedesktopAkonadiResourceInterface *> mResourceInterfaces;
};

} // namespace Server
} // namespace Akonadi

#endif
