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
#include <QStringList>
#include <QObject>
#include <QDBusConnection>

class QReadWriteLock;
class QWaitCondition;
class OrgFreedesktopAkonadiResourceInterface;

namespace Akonadi {
namespace Server {

class Collection;
class ItemRetrievalJob;
class ItemRetrievalRequest;

/** Manages and processes item retrieval requests. */
class ItemRetrievalManager : public AkThread
{
    Q_OBJECT
public:
    ItemRetrievalManager(QObject *parent = Q_NULLPTR);
    ~ItemRetrievalManager();

    void requestItemDelivery(qint64 uid, const QByteArray &remoteId, const QByteArray &mimeType,
                             const QString &resource, const QVector<QByteArray> &parts);

    /**
     * Added for convenience. ItemRetrievalManager takes ownership over the
     * pointer and deletes it when the request is processed.
     */
    void requestItemDelivery(ItemRetrievalRequest *request);

    static ItemRetrievalManager *instance();

Q_SIGNALS:
    void requestAdded();

private:
    OrgFreedesktopAkonadiResourceInterface *resourceInterface(const QString &id);

private Q_SLOTS:
    void init() Q_DECL_OVERRIDE;

    void serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);
    void processRequest();
    void triggerCollectionSync(const QString &resource, qint64 colId);
    void triggerCollectionTreeSync(const QString &resource);
    void retrievalJobFinished(ItemRetrievalRequest *request, const QString &errorMsg);

private:
    static ItemRetrievalManager *sInstance;
    /// Protects mPendingRequests and every Request object posted to it
    QReadWriteLock *mLock;
    /// Used to let requesting threads wait until the request has been processed
    QWaitCondition *mWaitCondition;
    /// Pending requests queues, one per resource
    QHash<QString, QList<ItemRetrievalRequest *> > mPendingRequests;
    /// Currently running jobs, one per resource
    QHash<QString, ItemRetrievalJob *> mCurrentJobs;

    // resource dbus interface cache
    QHash<QString, OrgFreedesktopAkonadiResourceInterface *> mResourceInterfaces;
};

} // namespace Server
} // namespace Akonadi

#endif
