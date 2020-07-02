/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
