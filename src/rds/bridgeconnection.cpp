/***************************************************************************
 *   SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "bridgeconnection.h"

#include <private/standarddirs_p.h>

#include <QDebug>
#include <QLocalSocket>
#include <QMetaObject>
#include <QRegularExpression>
#include <QSettings>
#include <QTcpSocket>

#ifdef Q_OS_UNIX
#include <sys/socket.h>
#include <sys/un.h>
#endif

BridgeConnection::BridgeConnection(QTcpSocket *remoteSocket, QObject *parent)
    : QObject(parent)
    , m_remoteSocket(remoteSocket)
{
    // wait for the vtable to be complete
    QMetaObject::invokeMethod(this, &BridgeConnection::doConnects, Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, &BridgeConnection::connectLocal, Qt::QueuedConnection);
}

BridgeConnection::~BridgeConnection()
{
    delete m_remoteSocket;
}

void BridgeConnection::slotDataAvailable()
{
    if (m_localSocket->bytesAvailable() > 0) {
        m_remoteSocket->write(m_localSocket->read(m_localSocket->bytesAvailable()));
    }
    if (m_remoteSocket->bytesAvailable() > 0) {
        m_localSocket->write(m_remoteSocket->read(m_remoteSocket->bytesAvailable()));
    }
}

AkonadiBridgeConnection::AkonadiBridgeConnection(QTcpSocket *remoteSocket, QObject *parent)
    : BridgeConnection(remoteSocket, parent)
{
    m_localSocket = new QLocalSocket(this);
}

void AkonadiBridgeConnection::connectLocal()
{
    const QSettings connectionSettings(Akonadi::StandardDirs::connectionConfigFile(), QSettings::IniFormat);
#ifdef Q_OS_WIN // krazy:exclude=cpp
    const QString namedPipe = connectionSettings.value(QLatin1String("Data/NamedPipe"), QLatin1String("Akonadi")).toString();
    (static_cast<QLocalSocket *>(m_localSocket))->connectToServer(namedPipe);
#else
    const QString defaultSocketDir = Akonadi::StandardDirs::saveDir("data");
    const QString path =
        connectionSettings.value(QStringLiteral("Data/UnixPath"), QString(defaultSocketDir + QLatin1String("/akonadiserver.socket"))).toString();
    (static_cast<QLocalSocket *>(m_localSocket))->connectToServer(path);
#endif
}

DBusBridgeConnection::DBusBridgeConnection(QTcpSocket *remoteSocket, QObject *parent)
    : BridgeConnection(remoteSocket, parent)
{
    m_localSocket = new QLocalSocket(this);
}

void DBusBridgeConnection::connectLocal()
{
    // TODO: support for !Linux
#ifdef Q_OS_UNIX
    const QByteArray sessionBusAddress = qgetenv("DBUS_SESSION_BUS_ADDRESS");
    const QRegularExpression rx(QStringLiteral("=(.*)[,$]"));
    QRegularExpressionMatch match = rx.match(QString::fromLatin1(sessionBusAddress));
    if (match.hasMatch()) {
        const QString dbusPath = match.captured(1);
        qDebug() << dbusPath;
        if (sessionBusAddress.contains("abstract")) {
            const int fd = socket(PF_UNIX, SOCK_STREAM, 0);
            Q_ASSERT(fd >= 0);
            struct sockaddr_un dbus_socket_addr;
            dbus_socket_addr.sun_family = PF_UNIX;
            dbus_socket_addr.sun_path[0] = '\0'; // this marks an abstract unix socket on linux, something QLocalSocket doesn't support
            memcpy(dbus_socket_addr.sun_path + 1, dbusPath.toLatin1().data(), dbusPath.toLatin1().size() + 1);
            /*sizeof(dbus_socket_addr) gives me a too large value for some reason, although that's what QLocalSocket uses*/
            const int result = ::connect(fd,
                                         reinterpret_cast<struct sockaddr *>(&dbus_socket_addr),
                                         sizeof(dbus_socket_addr.sun_family) + dbusPath.size() + 1 /* for the leading \0 */);
            Q_ASSERT(result != -1);
            Q_UNUSED(result) // in release mode
            (static_cast<QLocalSocket *>(m_localSocket))->setSocketDescriptor(fd, QLocalSocket::ConnectedState, QLocalSocket::ReadWrite);
        } else {
            (static_cast<QLocalSocket *>(m_localSocket))->connectToServer(dbusPath);
        }
    }
#endif
}

void BridgeConnection::doConnects()
{
    connect(m_localSocket, &QLocalSocket::disconnected, this, &BridgeConnection::deleteLater);
    connect(m_remoteSocket, &QAbstractSocket::disconnected, this, &QObject::deleteLater);
    connect(m_localSocket, &QIODevice::readyRead, this, &BridgeConnection::slotDataAvailable);
    connect(m_remoteSocket, &QIODevice::readyRead, this, &BridgeConnection::slotDataAvailable);
    connect(m_localSocket, &QLocalSocket::connected, this, &BridgeConnection::slotDataAvailable);
}
