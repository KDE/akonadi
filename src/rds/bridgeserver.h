/***************************************************************************
 *   Copyright (C) 2010 by Volker Krause <vkrause@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef BRIDGESERVER_H
#define BRIDGESERVER_H

#include <QtCore/QObject>
#include <QtNetwork/QTcpServer>

class BridgeServerBase : public QObject
{
    Q_OBJECT

public:
    explicit BridgeServerBase(quint16 port, QObject *parent = Q_NULLPTR);

protected Q_SLOTS:
    virtual void slotNewConnection() = 0;

protected:
    QTcpServer *m_server;
};

template <typename ConnectionType>
class BridgeServer : public BridgeServerBase
{
public:
    explicit BridgeServer(quint16 port, QObject *parent = Q_NULLPTR)
        : BridgeServerBase(port, parent)
    {
    }

protected:
    void slotNewConnection() Q_DECL_OVERRIDE
    {
        while (m_server->hasPendingConnections()) {
            new ConnectionType(m_server->nextPendingConnection(), this);
        }
    }
};

#endif // BRIDGESERVER_H
