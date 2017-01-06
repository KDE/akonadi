/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "agentthread.h"
#include "akonadiagentserver_debug.h"

#include <QCoreApplication>
#include <QPluginLoader>
#include <QWidget> // Needed for WId

#include <shared/akdebug.h>
#include <qmetaobject.h>

using namespace Akonadi;

AgentThread::AgentThread(const QString &identifier, QObject *factory, QObject *parent)
    : QThread(parent)
    , m_identifier(identifier)
    , m_factory(factory)
    , m_instance(nullptr)
{
}

void AgentThread::run()
{
    const bool invokeSucceeded = QMetaObject::invokeMethod(m_factory,
                                 "createInstance",
                                 Qt::DirectConnection,
                                 Q_RETURN_ARG(QObject *, m_instance),
                                 Q_ARG(QString, m_identifier));
    if (invokeSucceeded) {
        qCDebug(AKONADIAGENTSERVER_LOG) << Q_FUNC_INFO << "agent instance created: " << m_instance;
    } else {
        qCDebug(AKONADIAGENTSERVER_LOG) << Q_FUNC_INFO << "agent instance creation failed";
    }

    exec();
    delete m_instance;
}

void AgentThread::configure(qlonglong windowId)
{
    QMetaObject::invokeMethod(m_instance,
                              "configure",
                              Qt::DirectConnection,
                              Q_ARG(WId, (WId)windowId));
}
