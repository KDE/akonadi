/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "fakeclient.h"

#include <private/protocol_exception_p.h>
#include <private/protocol_p.h>
#include <private/datastream_p_p.h>

#include <QBuffer>
#include <QTest>
#include <QMutexLocker>
#include <QLocalSocket>

#define CLIENT_COMPARE(actual, expected, ...)\
do {\
    if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)) {\
        mSocket->disconnectFromServer();\
        return __VA_ARGS__;\
    }\
} while (0)

#define CLIENT_VERIFY(statement, ...)\
do {\
    if (!QTest::qVerify((statement), #statement, "", __FILE__, __LINE__)) {\
        mSocket->disconnectFromServer();\
        return __VA_ARGS__;\
    }\
} while (0)

using namespace Akonadi;
using namespace Akonadi::Server;

FakeClient::FakeClient(QObject *parent)
    : QThread(parent)
    , mMutex(QMutex::Recursive)
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

bool FakeClient::dataAvailable()
{
    QMutexLocker locker(&mMutex);

    CLIENT_VERIFY(!mScenarios.isEmpty(), false);

    readServerPart();
    writeClientPart();

    return true;
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
            QBuffer buffer(&scenario.data);
            buffer.open(QIODevice::ReadOnly);
            Protocol::DataStream expectedStream(&buffer);
            qint64 expectedTag;
            qint64 actualTag;

            expectedStream >> expectedTag;
            const auto expectedCommand = Protocol::deserialize(expectedStream.device());
            try {
                while (static_cast<size_t>(mSocket->bytesAvailable()) < sizeof(qint64)) {
                    Protocol::DataStream::waitForData(mSocket, 5000);
                }
            } catch (const ProtocolException &e) {
                qDebug() << "ProtocolException:" << e.what();
                qDebug() << "Expected response:" << Protocol::debugString(expectedCommand);
                CLIENT_VERIFY(false);
            }

            mStream >> actualTag;
            CLIENT_COMPARE(actualTag, expectedTag);

            Protocol::CommandPtr actualCommand;
            try {
                actualCommand = Protocol::deserialize(mStream.device());
            } catch (const ProtocolException &e) {
                qDebug() << "Protocol exception:" << e.what();
                qDebug() << "Expected response:" << Protocol::debugString(expectedCommand);
                CLIENT_VERIFY(false);
            }

            if (actualCommand->type() != expectedCommand->type()) {
                qDebug() << "Actual command:  " << Protocol::debugString(actualCommand);
                qDebug() << "Expected Command:" << Protocol::debugString(expectedCommand);
            }
            CLIENT_COMPARE(actualCommand->type(), expectedCommand->type());
            CLIENT_COMPARE(actualCommand->isResponse(), expectedCommand->isResponse());
            if (*actualCommand != *expectedCommand) {
                qDebug() << "Actual command:  " << Protocol::debugString(actualCommand);
                qDebug() << "Expected Command:" << Protocol::debugString(expectedCommand);
            }

            CLIENT_COMPARE(*actualCommand, *expectedCommand);
        }
    }

    if (!mScenarios.isEmpty()) {
        CLIENT_VERIFY(mScenarios.at(0).action == TestScenario::ClientCmd
                        || mScenarios.at(0).action == TestScenario::Wait
                        || mScenarios.at(0).action ==TestScenario::Quit);
    } else {
        // Server replied and there's nothing else to send, then quit
        mSocket->disconnectFromServer();
    }
}

void FakeClient::writeClientPart()
{
    while (!mScenarios.isEmpty() && (mScenarios.at(0).action == TestScenario::ClientCmd
            || mScenarios.at(0).action == TestScenario::Wait)) {
        const TestScenario rule = mScenarios.takeFirst();

       if (rule.action == TestScenario::ClientCmd) {
            mSocket->write(rule.data);
            CLIENT_VERIFY(mSocket->waitForBytesWritten());
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
    connect(mSocket, &QLocalSocket::disconnected, this, &FakeClient::connectionLost);
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    connect(mSocket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error),
#else
    connect(mSocket, &QLocalSocket::errorOccurred,
#endif
            this, [this]() {
                qWarning() << "Client socket error: " << mSocket->errorString();
                connectionLost();
                QVERIFY(false);
            });
    if (!mSocket->waitForConnected()) {
        qFatal("Failed to connect to FakeAkonadiServer");
        QVERIFY(false);
        return;
    }
    mStream.setDevice(mSocket);

    Q_FOREVER {
        if (mSocket->state() != QLocalSocket::ConnectedState) {
            connectionLost();
            break;
        }

        {
            QEventLoop loop;
            connect(mSocket, &QLocalSocket::readyRead, &loop, &QEventLoop::quit);
            connect(mSocket, &QLocalSocket::disconnected, &loop, &QEventLoop::quit);
            loop.exec();
        }

        while (mSocket->bytesAvailable() > 0) {
            if (mSocket->state() != QLocalSocket::ConnectedState) {
                connectionLost();
                break;
            }

            if(!dataAvailable()) {
                break;
            }
        }
    }

    mStream.setDevice(nullptr);
    mSocket->close();
    delete mSocket;
    mSocket = nullptr;
}

void FakeClient::connectionLost()
{
    // Otherwise this is an error on server-side, we expected more talking
    CLIENT_VERIFY(isScenarioDone());
}
