/***************************************************************************
 *   SPDX-FileCopyrightText: 2013 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "session.h"

#include <private/standarddirs_p.h>

#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include <QSocketNotifier>

#include <fcntl.h>
#include <iostream>
#include <unistd.h>

Session::Session(const QString &input, QObject *parent)
    : QObject(parent)
{
    auto file = new QFile(this);
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

    auto socket = new QLocalSocket(this);
    connect(socket, &QLocalSocket::errorOccurred, this, &Session::serverError);
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
