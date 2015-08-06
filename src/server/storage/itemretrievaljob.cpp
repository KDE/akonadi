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

#include <shared/akdebug.h>

#include <QDBusPendingCallWatcher>
#include <QDBusError>

using namespace Akonadi::Server;

ItemRetrievalJob::~ItemRetrievalJob()
{
    Q_ASSERT(!m_active);
}

void ItemRetrievalJob::start(org::freedesktop::Akonadi::Resource *interface)
{
    Q_ASSERT(m_request);
    akDebug() << "processing retrieval request for item" << m_request->id << " parts:" << m_request->parts << " of resource:" << m_request->resourceId;

    m_interface = interface;
    // call the resource
    if (interface) {
        m_active = true;
        auto reply = interface->requestItemDelivery(m_request->id, m_request->remoteId,
                                                    m_request->mimeType, m_request->parts);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
        connect(watcher, &QDBusPendingCallWatcher::finished,
                this, &ItemRetrievalJob::callFinished);
    } else {
        Q_EMIT requestCompleted(m_request, QStringLiteral("Unable to contact resource"));
        deleteLater();
    }
}

void ItemRetrievalJob::kill()
{
    m_active = false;
    Q_EMIT requestCompleted(m_request, QStringLiteral("Request cancelled"));
}

void ItemRetrievalJob::callFinished(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    QDBusPendingReply<QString> reply = *watcher;
    if (m_active) {
        m_active = false;
        const QString errorMsg = reply.isError() ? reply.error().message() : reply;
        if (!errorMsg.isEmpty()) {
            Q_EMIT requestCompleted(m_request, QStringLiteral("Unable to retrieve item from resource: %1").arg(errorMsg));
        } else {
            Q_EMIT requestCompleted(m_request, QString());
        }
    }
    deleteLater();
}
