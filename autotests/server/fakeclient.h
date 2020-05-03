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

#ifndef AKONADI_SERVER_FAKECLIENT_H
#define AKONADI_SERVER_FAKECLIENT_H

#include <QThread>
#include <QMutex>
#include "datastream_p_p.h"

#include "fakeakonadiserver.h"

class QLocalSocket;

namespace Akonadi {
namespace Server {

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
    mutable QMutex mMutex;

    TestScenario::List mScenarios;
    QLocalSocket *mSocket = nullptr;
    Protocol::DataStream mStream;
};
}
}


#endif // AKONADI_SERVER_FAKECLIENT_H
