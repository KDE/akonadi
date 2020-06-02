/***************************************************************************
 *   Copyright (C) 2013 by Volker Krause <vkrause@kde.org>                 *
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

#include "session.h"

#include <private/standarddirs_p.h>

#include <QCoreApplication>
#include <QFile>
#include <QSocketNotifier>
#include <QSettings>
#include <QLocalSocket>

#include <iostream>
#include <fcntl.h>
#include <unistd.h>

Session::Session(const QString &input, QObject *parent)
    : QObject(parent)
{
    QFile *file = new QFile(this);
    if (input != QLatin1String("-")) {
        file->setFileName(input);
        if (!file->open(QFile::ReadOnly)) {
            qFatal("Failed to open %s", qPrintable(input));
        }
    } else {
        // ### does that work on Windows?
        const int flags = fcntl(0, F_GETFL);
        fcntl(0, F_SETFL, flags | O_NONBLOCK); // NOLINT(hicpp-signed-bitwise)

        if (!file->open(stdin, QFile::ReadOnly | QFile::Unbuffered)) {
            qFatal("Failed to open stdin!");
        }
        m_notifier = new QSocketNotifier(0, QSocketNotifier::Read, this);
        connect(m_notifier, &QSocketNotifier::activated, this, &Session::inputAvailable);
    }
    m_input = file;
}

Session::~Session()
{
}

void Session::connectToHost()
{
    const QSettings connectionSettings(Akonadi::StandardDirs::connectionConfigFile(), QSettings::IniFormat);

    QString serverAddress;
#ifdef Q_OS_WIN
    serverAddress = connectionSettings.value(QStringLiteral("Data/NamedPipe"), QString()).toString();
#else
    serverAddress = connectionSettings.value(QStringLiteral("Data/UnixPath"), QString()).toString();
#endif
    if (serverAddress.isEmpty()) {
        qFatal("Unable to determine server address.");
    }

    QLocalSocket *socket = new QLocalSocket(this);
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    connect(socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error), this, &Session::serverError);
#else
    connect(socket, &QLocalSocket::errorOccurred, this, &Session::serverError);
#endif
    connect(socket, &QLocalSocket::disconnected, this, &Session::serverDisconnected);
    connect(socket, &QIODevice::readyRead, this, &Session::serverRead);
    connect(socket, &QLocalSocket::connected, this, &Session::inputAvailable);

    m_session = socket;
    socket->connectToServer(serverAddress);

    m_connectionTime.start();
}

void Session::inputAvailable()
{
    if (!m_session->isOpen()) {
        return;
    }

    if (m_notifier) {
        m_notifier->setEnabled(false);
    }

    if (m_input->atEnd()) {
        return;
    }

    QByteArray buffer(1024, Qt::Uninitialized);
    qint64 readSize = 0;

    while ((readSize = m_input->read(buffer.data(), buffer.size())) > 0) {
        m_session->write(buffer.constData(), readSize);
        m_sentBytes += readSize;
    }

    if (m_notifier) {
        m_notifier->setEnabled(true);
    }
}

void Session::serverDisconnected()
{
    QCoreApplication::exit(0);
}

void Session::serverError(QLocalSocket::LocalSocketError socketError)
{
    if (socketError == QLocalSocket::PeerClosedError) {
        QCoreApplication::exit(0);
        return;
    }

    std::cerr << qPrintable(m_session->errorString());
    QCoreApplication::exit(1);
}

void Session::serverRead()
{
    QByteArray buffer(1024, Qt::Uninitialized);
    qint64 readSize = 0;

    while ((readSize = m_session->read(buffer.data(), buffer.size())) > 0) {
        write(1, buffer.data(), readSize);
        m_receivedBytes += readSize;
    }
}

void Session::printStats() const
{
    std::cerr << "Connection time: " << m_connectionTime.elapsed() << " ms" << std::endl;
    std::cerr << "Sent: " << m_sentBytes << " bytes" << std::endl;
    std::cerr << "Received: " << m_receivedBytes << " bytes" << std::endl;
}
