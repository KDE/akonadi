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

#include <private/protocol_p.h>

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

using namespace Akonadi;
using namespace Akonadi::Server;

FakeClient::FakeClient(QObject *parent)
    : QThread(parent)
{
    moveToThread(this);
}

FakeClient::~FakeClient()
{
}

void FakeClient::setScenarios(const TestScenario::List &scenarios)
{
    mScenarios = scenarios;
}

bool FakeClient::isScenarioDone() const
{
    QMutexLocker locker(&mMutex);
    return mScenarios.isEmpty();
}

void FakeClient::dataAvailable()
{
    QMutexLocker locker(&mMutex);

    CLIENT_VERIFY(!mScenarios.isEmpty());

    readServerPart();
    writeClientPart();
}

void FakeClient::readServerPart()
{
    while (!mScenarios.isEmpty() && (mScenarios.at(0).action == TestScenario::ServerCmd
            || mScenarios.at(0).action == TestScenario::Ignore)) {
        TestScenario scenario = mScenarios.takeFirst();
        if (scenario.action == TestScenario::Ignore) {
            const int count = scenario.data.toInt();

            // Read and throw away all "count" responses. Useful for scenarios
            // with thousands of responses
            qint64 tag;
            for (int i = 0; i < count; ++i) {
                mStream >> tag;
                Protocol::deserialize(mStream.device());
            }
        } else {
            QDataStream expectedStream(scenario.data);
            qint64 expectedTag, actualTag;
            Protocol::Command expectedCommand, actualCommand;

            expectedStream >> expectedTag;

            while ((size_t)mSocket->bytesAvailable() < sizeof(qint64)) {
                mSocket->waitForReadyRead();
            }
            mStream >> actualTag;
            CLIENT_COMPARE(actualTag, expectedTag);

            expectedCommand = Protocol::deserialize(expectedStream.device());
            actualCommand = Protocol::deserialize(mStream.device());

            if (actualCommand.type() != expectedCommand.type()) {
                qDebug() << "Actual command:  " << actualCommand.debugString();
                qDebug() << "Expected Command:" << expectedCommand.debugString();
            }
            CLIENT_COMPARE(actualCommand.type(), expectedCommand.type());
            CLIENT_COMPARE(actualCommand.isResponse(), expectedCommand.isResponse());
            if (actualCommand != expectedCommand) {
                qDebug() << "Actual command:  " << actualCommand.debugString();
                qDebug() << "Expected Command:" << expectedCommand.debugString();
            }

            CLIENT_COMPARE(actualCommand, expectedCommand);
        }
    }

    if (!mScenarios.isEmpty()) {
        CLIENT_VERIFY(mScenarios.at(0).action == TestScenario::ClientCmd
                        || mScenarios.at(0).action == TestScenario::Wait
                        || mScenarios.at(0).action ==TestScenario::Quit);
    } else {
        // Server replied and there's nothing else to send, then quit
        quit();
    }
}

void FakeClient::writeClientPart()
{
    while (!mScenarios.isEmpty() && (mScenarios.at(0).action == TestScenario::ClientCmd
            || mScenarios.at(0).action == TestScenario::Wait)) {
        const TestScenario rule = mScenarios.takeFirst();

       if (rule.action == TestScenario::ClientCmd) {
            mSocket->write(rule.data);
        } else {
            const int timeout = rule.data.toInt();
            QTest::qWait(timeout);
        }
    }

    if (!mScenarios.isEmpty() && mScenarios.at(0).action == TestScenario::Quit) {
        mSocket->close();
    }

    if (!mScenarios.isEmpty()) {
        CLIENT_VERIFY(mScenarios.at(0).action == TestScenario::ServerCmd
                        || mScenarios.at(0).action == TestScenario::Ignore);
    }
}

void FakeClient::run()
{
    mSocket = new QLocalSocket();
    mSocket->connectToServer(FakeAkonadiServer::socketFile());
    connect(mSocket, &QIODevice::readyRead, this, &FakeClient::dataAvailable);
    connect(mSocket, &QLocalSocket::disconnected, this, &FakeClient::connectionLost);
    if (!mSocket->waitForConnected()) {
        qFatal("Failed to connect to FakeAkonadiServer");
    }
    mStream.setDevice(mSocket);

    exec();

    mStream.setDevice(Q_NULLPTR);
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
