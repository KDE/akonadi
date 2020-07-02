/***************************************************************************
 *   SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
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
