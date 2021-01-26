/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef AKONADI_SERVER_FAKECLIENT_H
#define AKONADI_SERVER_FAKECLIENT_H

#include "datastream_p_p.h"
#include <QRecursiveMutex>
#include <QThread>

#include "fakeakonadiserver.h"

class QLocalSocket;

namespace Akonadi
{
namespace Server
{
class FakeClient : public QThread
{
    Q_OBJECT

public:
    explicit FakeClient(QObject *parent = nullptr);
    ~FakeClient() override;

    void setScenarios(const TestScenario::List &scenarios);

    bool isScenarioDone() const;

protected:
    void run() override;

private Q_SLOTS:
    bool dataAvailable();
    void readServerPart();
    void writeClientPart();
    void connectionLost();

private:
    mutable QRecursiveMutex mMutex;

    TestScenario::List mScenarios;
    QLocalSocket *mSocket = nullptr;
    Protocol::DataStream mStream;
};
}
}

#endif // AKONADI_SERVER_FAKECLIENT_H
