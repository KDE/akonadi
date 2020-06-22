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

#ifndef BRIDGECONNECTION_H
#define BRIDGECONNECTION_H

#include <QObject>

class QTcpSocket;
class QLocalSocket;

class BridgeConnection : public QObject
{
    Q_OBJECT

public:
    explicit BridgeConnection(QTcpSocket *remoteSocket, QObject *parent = nullptr);
    ~BridgeConnection();

protected Q_SLOTS:
    virtual void connectLocal() = 0;
    void doConnects();

protected:
    QLocalSocket *m_localSocket = nullptr;

private Q_SLOTS:
    void slotDataAvailable();

private:
    QTcpSocket *m_remoteSocket = nullptr;
};

class AkonadiBridgeConnection : public BridgeConnection
{
    Q_OBJECT

public:
    explicit AkonadiBridgeConnection(QTcpSocket *remoteSocket, QObject *parent = nullptr);

protected:
    void connectLocal() override;
};

class DBusBridgeConnection : public BridgeConnection
{
    Q_OBJECT

public:
    explicit DBusBridgeConnection(QTcpSocket *remoteSocket, QObject *parent = nullptr);

protected:
    void connectLocal() override;
};

#endif // BRIDGECONNECTION_H
