/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "fakeclient.h"
#include "fakeakonadiserver.h"
#include "imapstreamparser.h"
#include "akdebug.h"

#include <QTest>
#include <QMutexLocker>
#include <QLocalSocket>

#define CLIENT_COMPARE(actual, expected)\
do {\
    if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)) {\
        quit();\
        return;\
    }\
} while (0)

#define CLIENT_VERIFY(statement)\
do {\
    if (!QTest::qVerify((statement), #statement, "", __FILE__, __LINE__)) {\
        quit();\
        return;\
    }\
} while (0)

using namespace Akonadi::Server;

FakeClient::FakeClient(QObject *parent)
    : QThread(parent)
{
    moveToThread(this);
}

FakeClient::~FakeClient()
{
}

void FakeClient::setScenario(const QList<QByteArray> &scenario)
{
    mScenario = scenario;
}

bool FakeClient::isScenarioDone() const
{
    QMutexLocker locker(&mMutex);
    return mScenario.isEmpty();
}

void FakeClient::dataAvailable()
{
    QMutexLocker locker(&mMutex);

    CLIENT_VERIFY(!mScenario.isEmpty());

    readServerPart();
    writeClientPart();
}

void FakeClient::readServerPart()
{
    while (!mScenario.isEmpty() && mScenario.first().startsWith("S: ")) {
        const QByteArray received = "S: " + mStreamParser->readUntilCommandEnd();
        const QByteArray expected = mScenario.takeFirst() + "\r\n";
        CLIENT_COMPARE(QString::fromUtf8(received), QString::fromUtf8(expected));
        CLIENT_COMPARE(received, expected);
    }

    if (!mScenario.isEmpty()) {
        CLIENT_VERIFY(mScenario.first().startsWith("C: "));
    } else {
        // Server replied and there's nothing else to send, then quit
        quit();
    }
}

void FakeClient::writeClientPart()
{
    while (!mScenario.isEmpty() && (mScenario.first().startsWith("C: ") || mScenario.first().startsWith("W: "))) {
        const QByteArray rule = mScenario.takeFirst();

        if (rule.startsWith("C: ")) {
            const QByteArray payload = rule.mid(3);
            mSocket->write(payload + "\n");
        } else {
            const int timeout = rule.mid(3).toInt();
            QTest::qWait(timeout);
        }
    }

    if (!mScenario.isEmpty() && mScenario.first().startsWith("X:")) {
        mSocket->close();
    }

    if (!mScenario.isEmpty()) {
        CLIENT_VERIFY(mScenario.first().startsWith("S: "));
    }
}

void FakeClient::run()
{
    mSocket = new QLocalSocket();
    mSocket->connectToServer(FakeAkonadiServer::socketFile());
    connect(mSocket, SIGNAL(readyRead()), this, SLOT(dataAvailable()));
    connect(mSocket, SIGNAL(disconnected()), this, SLOT(connectionLost()));
    if (!mSocket->waitForConnected()) {
        akFatal() << "Failed to connect to FakeAkonadiServer";
    }
    mStreamParser = new ImapStreamParser(mSocket);

    exec();

    delete mStreamParser;
    mStreamParser = 0;
    mSocket->close();
    delete mSocket;
    mSocket = 0;
}

void FakeClient::connectionLost()
{
    // Otherwise this is an error on server-side, we expected more talking
    CLIENT_VERIFY(isScenarioDone());

    quit();
}
