/***************************************************************************
 *   SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#pragma once

#include <QObject>
#include <QTcpServer>

class BridgeServerBase : public QObject
{
    Q_OBJECT

public:
    explicit BridgeServerBase(quint16 port, QObject *parent = nullptr);

protected Q_SLOTS:
    virtual void slotNewConnection() = 0;

protected:
    QTcpServer *const m_server;
};

template<typename ConnectionType> class BridgeServer : public BridgeServerBase
{
public:
    explicit BridgeServer(quint16 port, QObject *parent = nullptr)
        : BridgeServerBase(port, parent)
    {
    }

protected:
    void slotNewConnection() override
    {
        while (m_server->hasPendingConnections()) {
            new ConnectionType(m_server->nextPendingConnection(), this);
        }
    }
};

