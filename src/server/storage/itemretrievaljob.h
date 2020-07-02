/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef ITEMRETRIEVALJOB_H
#define ITEMRETRIEVALJOB_H

#include <QObject>

#include "itemretrievalrequest.h"

class QDBusPendingCallWatcher;
class OrgFreedesktopAkonadiResourceInterface;

namespace Akonadi
{
namespace Server
{

class ItemRetrievalRequest;

class AbstractItemRetrievalJob : public QObject
{
    Q_OBJECT
public:
    AbstractItemRetrievalJob(ItemRetrievalRequest req, QObject *parent);
    ~AbstractItemRetrievalJob() override = default;

    virtual void start() = 0;
    virtual void kill() = 0;

    const ItemRetrievalRequest &request() const { return m_result.request; }
    const ItemRetrievalResult &result() const { return m_result; }

Q_SIGNALS:
    void requestCompleted(Akonadi::Server::AbstractItemRetrievalJob *job);

protected:
    ItemRetrievalResult m_result;
};

/// Async D-Bus retrieval, no modification of the request (thus no need for locking)
class ItemRetrievalJob : public AbstractItemRetrievalJob
{
    Q_OBJECT
public:
    ItemRetrievalJob(ItemRetrievalRequest req, QObject *parent)
        : AbstractItemRetrievalJob(std::move(req), parent)
    {}

    void setInterface(OrgFreedesktopAkonadiResourceInterface *interface)
    {
        m_interface = interface;
    }

    ~ItemRetrievalJob() override;
    void start() override;
    void kill() override;

private Q_SLOTS:
    void callFinished(QDBusPendingCallWatcher *watcher);

private:
    bool m_active = false;
    OrgFreedesktopAkonadiResourceInterface *m_interface = nullptr;

};

} // namespace Server
} // namespace Akonadi

#endif
