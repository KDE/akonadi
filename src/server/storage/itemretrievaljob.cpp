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

#include "itemretrievaljob.h"
#include "itemretrievalrequest.h"
#include "resourceinterface.h"
#include "akonadiserver_debug.h"

#include <QDBusPendingCallWatcher>

using namespace Akonadi::Server;

AbstractItemRetrievalJob::AbstractItemRetrievalJob(ItemRetrievalRequest req, QObject *parent)
    : QObject(parent)
    , m_result(std::move(req))
{}

ItemRetrievalJob::~ItemRetrievalJob()
{
    Q_ASSERT(!m_active);
}

void ItemRetrievalJob::start()
{
    qCDebug(AKONADISERVER_LOG) << "processing retrieval request for item" << request().ids << " parts:" << request().parts << " of resource:" << request().resourceId;

    // call the resource
    if (m_interface) {
        m_active = true;
        auto reply = m_interface->requestItemDelivery(request().ids, request().parts);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
        connect(watcher, &QDBusPendingCallWatcher::finished,
                this, &ItemRetrievalJob::callFinished);
    } else {
        m_result.errorMsg = QStringLiteral("Unable to contact resource");
        Q_EMIT requestCompleted(this);
        deleteLater();
    }
}

void ItemRetrievalJob::kill()
{
    m_active = false;
    m_result.errorMsg = QStringLiteral("Request cancelled");
    Q_EMIT requestCompleted(this);
}

void ItemRetrievalJob::callFinished(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    QDBusPendingReply<void> reply = *watcher;
    if (m_active) {
        m_active = false;
        if (reply.isError()) {
            m_result.errorMsg = QStringLiteral("Unable to retrieve item from resource: %1").arg(reply.error().message());
        }
        Q_EMIT requestCompleted(this);
    }
    deleteLater();
}
