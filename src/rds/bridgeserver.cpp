/***************************************************************************
 *   SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "bridgeserver.h"

#include "exception.h"

BridgeServerBase::BridgeServerBase(quint16 port, QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &BridgeServerBase::slotNewConnection);
    if (!m_server->listen(QHostAddress::Any, port)) {
        throw Exception<std::runtime_error>(QStringLiteral("Can't listen to port %1: %2")
                                            .arg(port).arg(m_server->errorString()));
    }
}
