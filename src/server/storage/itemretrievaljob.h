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

class QDBusError;
class QDBusPendingCallWatcher;
class OrgFreedesktopAkonadiResourceInterface;

namespace Akonadi {
namespace Server {

class ItemRetrievalRequest;

/// Async D-Bus retrieval, no modification of the request (thus no need for locking)
class ItemRetrievalJob : public QObject
{
    Q_OBJECT
public:
    ItemRetrievalJob(ItemRetrievalRequest *req, QObject *parent)
        : QObject(parent)
        , m_request(req)
        , m_active(false)
        , m_interface(0)
    {
    }
    ~ItemRetrievalJob();
    void start(OrgFreedesktopAkonadiResourceInterface *interface);
    void kill();

Q_SIGNALS:
    void requestCompleted(ItemRetrievalRequest *req, const QString &errorMsg);

private:
    void callFinished(QDBusPendingCallWatcher *watcher);
    ItemRetrievalRequest *m_request;
    bool m_active;
    OrgFreedesktopAkonadiResourceInterface *m_interface;

};

} // namespace Server
} // namespace Akonadi

#endif
