/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentthread.h"
#include "akonadiagentserver_debug.h"

#include "shared/akdebug.h"
#include <QMetaObject>

using namespace Akonadi;

AgentThread::AgentThread(const QString &identifier, QObject *factory, QObject *parent)
    : QThread(parent)
    , m_identifier(identifier)
    , m_factory(factory)
{
}

void AgentThread::run()
{
    // // clang-format off
    const bool invokeSucceeded =
        QMetaObject::invokeMethod(m_factory, "createInstance", Qt::DirectConnection, Q_RETURN_ARG(QObject *, m_instance), Q_ARG(QString, m_identifier));
    // clang-format on
    if (invokeSucceeded) {
        qCDebug(AKONADIAGENTSERVER_LOG) << Q_FUNC_INFO << "agent instance created: " << m_instance;
    } else {
        qCDebug(AKONADIAGENTSERVER_LOG) << Q_FUNC_INFO << "agent instance creation failed";
    }

    exec();
    delete m_instance;
    m_instance = nullptr;
}

void AgentThread::configure(qlonglong windowId)
{
    QMetaObject::invokeMethod(m_instance, "configure", Qt::DirectConnection, Q_ARG(quintptr, (quintptr)windowId));
}

#include "moc_agentthread.cpp"
