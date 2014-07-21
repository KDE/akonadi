/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include <QObject>

#include <handler/list.h>
#include <imapstreamparser.h>
#include <response.h>

#include "fakeakonadiserver.h"
#include "aktest.h"
#include "akdebug.h"
#include "entities.h"
#include "dbinitializer.h"

#include <QtTest/QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

class ListHandlerTest : public QObject
{
    Q_OBJECT

public:
    ListHandlerTest()
        : QObject()
    {
        qRegisterMetaType<Akonadi::Server::Response>();

        try {
            FakeAkonadiServer::instance()->setPopulateDb(false);
            FakeAkonadiServer::instance()->init();
        } catch (const FakeAkonadiServerException &e) {
            akError() << "Server exception: " << e.what();
            akFatal() << "Fake Akonadi Server failed to start up, aborting test";
        }
    }

    ~ListHandlerTest()
    {
        FakeAkonadiServer::instance()->quit();
    }

    QScopedPointer<DbInitializer> initializer;
private Q_SLOTS:

    void testList_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");
        Collection col2 = initializer->createCollection("col2", col1);
        Collection col3 = initializer->createCollection("col3", col2);

        QTest::addColumn<QList<QByteArray> >("scenario");

        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST 0 INF () ()"
                    << initializer->listResponse(initializer->collection("Search"))
                    << initializer->listResponse(col3)
                    << initializer->listResponse(col2)
                    << initializer->listResponse(col1)
                     << "S: 2 OK List completed";
            QTest::newRow("recursive list") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                    << "C: 2 LIST " + QByteArray::number(col1.id()) + " 0 () ()"
                    << initializer->listResponse(col1)
                     << "S: 2 OK List completed";
            QTest::newRow("base list") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                    << "C: 2 LIST " + QByteArray::number(col1.id()) + " 1 () ()"
                    << initializer->listResponse(col2)
                     << "S: 2 OK List completed";
            QTest::newRow("first level list") << scenario;
        }
    }

    void testList()
    {
        QFETCH(QList<QByteArray>, scenario);

        FakeAkonadiServer::instance()->setScenario(scenario);
        FakeAkonadiServer::instance()->runTest();
    }

    void testListEnabled_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");
        Collection col2 = initializer->createCollection("col2", col1);
        col2.setEnabled(false);
        col2.setSyncPref(Akonadi::Server::Tristate::True);
        col2.setDisplayPref(Akonadi::Server::Tristate::True);
        col2.setIndexPref(Akonadi::Server::Tristate::True);
        col2.update();
        Collection col3 = initializer->createCollection("col3", col2);
        col3.setEnabled(true);
        col3.setSyncPref(Akonadi::Server::Tristate::False);
        col3.setDisplayPref(Akonadi::Server::Tristate::False);
        col3.setIndexPref(Akonadi::Server::Tristate::False);
        col3.update();

        QTest::addColumn<QList<QByteArray> >("scenario");

        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST 0 INF (DISPLAY  ) ()"
                    << initializer->listResponse(initializer->collection("Search"))
                    << initializer->listResponse(col2)
                    << initializer->listResponse(col1)
                     << "S: 2 OK List completed";
            QTest::newRow("recursive list to display including local override") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST 0 INF (SYNC  ) ()"
                    << initializer->listResponse(initializer->collection("Search"))
                    << initializer->listResponse(col2)
                    << initializer->listResponse(col1)
                     << "S: 2 OK List completed";
            QTest::newRow("recursive list to sync including local override") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST 0 INF (INDEX  ) ()"
                    << initializer->listResponse(initializer->collection("Search"))
                    << initializer->listResponse(col2)
                    << initializer->listResponse(col1)
                     << "S: 2 OK List completed";
            QTest::newRow("recursive list to index including local override") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST 0 INF (ENABLED  ) ()"
                    << initializer->listResponse(initializer->collection("Search"))
                    // << initializer->listResponse(col3)
                    << initializer->listResponse(col1)
                     << "S: 2 OK List completed";
            QTest::newRow("recursive list of enabled") << scenario;
        }
    }

    void testListEnabled()
    {
        QFETCH(QList<QByteArray>, scenario);

        FakeAkonadiServer::instance()->setScenario(scenario);
        FakeAkonadiServer::instance()->runTest();
    }

};

AKTEST_FAKESERVER_MAIN(ListHandlerTest)

#include "listhandlertest.moc"
