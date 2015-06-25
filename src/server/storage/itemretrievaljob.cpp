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

#include <shared/akdebug.h>

#include <QDBusAbstractInterface>

using namespace Akonadi::Server;

ItemRetrievalJob::~ItemRetrievalJob()
{
    Q_ASSERT(!m_active);
}

void ItemRetrievalJob::start(QDBusAbstractInterface *interface)
{
    Q_ASSERT(m_request);
    akDebug() << "processing retrieval request for item" << m_request->id << " parts:" << m_request->parts << " of resource:" << m_request->resourceId;

    m_interface = interface;
    // call the resource
    if (interface) {
        m_active = true;
        m_oldMethodCalled = false;
        QList<QVariant> arguments;
        // TODO: Change the DBus call
        QStringList parts;
        parts.reserve(m_request->parts.size());
        Q_FOREACH (const QByteArray &part, m_request->parts) {
            parts << QLatin1String(part);
        }
        arguments << m_request->id
                  << QString::fromUtf8(m_request->remoteId)
                  << QString::fromUtf8(m_request->mimeType)
                  << parts;
        interface->callWithCallback(QLatin1String("requestItemDeliveryV2"), arguments, this, SLOT(callFinished(QString)), SLOT(callFailed(QDBusError)));
    } else {
        Q_EMIT requestCompleted(m_request, QString::fromLatin1("Unable to contact resource"));
        deleteLater();
    }
}

void ItemRetrievalJob::kill()
{
    m_active = false;
    Q_EMIT requestCompleted(m_request, QLatin1String("Request cancelled"));
}

void ItemRetrievalJob::callFinished(bool returnValue)
{
    if (m_active) {
        m_active = false;
        if (!returnValue) {
            Q_EMIT requestCompleted(m_request, QString::fromLatin1("Resource was unable to deliver item"));
        } else {
            Q_EMIT requestCompleted(m_request, QString());
        }
    }
    deleteLater();
}

void ItemRetrievalJob::callFinished(const QString &errorMsg)
{
    if (m_active) {
        m_active = false;
        if (!errorMsg.isEmpty()) {
            Q_EMIT requestCompleted(m_request, QString::fromLatin1("Unable to retrieve item from resource: %1").arg(errorMsg));
        } else {
            Q_EMIT requestCompleted(m_request, QString());
        }
    }
    deleteLater();
}

void ItemRetrievalJob::callFailed(const QDBusError &error)
{
    if (error.type() == QDBusError::UnknownMethod && !m_oldMethodCalled) {
        akDebug() << "processing retrieval request (old method) for item" << m_request->id << " parts:" << m_request->parts << " of resource:" << m_request->resourceId;
        Q_ASSERT(m_interface);
        // TODO: Remove?
        //try the old version
        QList<QVariant> arguments;
        QStringList parts;
        parts.reserve(m_request->parts.size());
        Q_FOREACH (const QByteArray &part, m_request->parts) {
            parts << QLatin1String(part);
        }
        arguments << m_request->id
                  << QString::fromUtf8(m_request->remoteId)
                  << QString::fromUtf8(m_request->mimeType)
                  << parts;
        m_oldMethodCalled = true;
        m_interface->callWithCallback(QLatin1String("requestItemDelivery"), arguments, this, SLOT(callFinished(bool)), SLOT(callFailed(QDBusError)));
        return;
    }
    if (m_active) {
        m_active = false;
        Q_EMIT requestCompleted(m_request, QString::fromLatin1("Unable to retrieve item from resource: %1").arg(error.message()));
    }
    deleteLater();
}
