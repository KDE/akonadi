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

#ifndef ITEMRETRIEVALJOB_H
#define ITEMRETRIEVALJOB_H

#include <QObject>

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
    AbstractItemRetrievalJob(ItemRetrievalRequest *req, QObject *parent);
    virtual ~AbstractItemRetrievalJob();

    virtual void start() = 0;
    virtual void kill() = 0;

Q_SIGNALS:
    void requestCompleted(ItemRetrievalRequest *request, const QString &errorMsg);

protected:
    ItemRetrievalRequest *m_request;
};

/// Async D-Bus retrieval, no modification of the request (thus no need for locking)
class ItemRetrievalJob : public AbstractItemRetrievalJob
{
    Q_OBJECT
public:
    ItemRetrievalJob(ItemRetrievalRequest *req, QObject *parent)
        : AbstractItemRetrievalJob(req, parent)
        , m_active(false)
        , m_interface(nullptr)
    {
    }

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
    bool m_active;
    OrgFreedesktopAkonadiResourceInterface *m_interface;

};

} // namespace Server
} // namespace Akonadi

#endif
