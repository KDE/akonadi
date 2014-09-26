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

Q_DECLARE_METATYPE(Akonadi::Server::Tag::List);
Q_DECLARE_METATYPE(Akonadi::Server::Tag);


class FetchHandlerTest : public QObject
{
    Q_OBJECT

public:
    FetchHandlerTest()
        : QObject()
    {
        qRegisterMetaType<Akonadi::Server::Response>();
        qRegisterMetaType<Akonadi::Server::Tag::List>();

        try {
            FakeAkonadiServer::instance()->setPopulateDb(false);
            FakeAkonadiServer::instance()->init();
        } catch (const FakeAkonadiServerException &e) {
            akError() << "Server exception: " << e.what();
            akFatal() << "Fake Akonadi Server failed to start up, aborting test";
        }
    }

    ~FetchHandlerTest()
    {
        FakeAkonadiServer::instance()->quit();
    }

    QScopedPointer<DbInitializer> initializer;

private Q_SLOTS:
    void testFetch_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col = initializer->createCollection("root");
        PimItem item1 = initializer->createItem("item1", col);
        PimItem item2 = initializer->createItem("item2", col);

        QTest::addColumn<QList<QByteArray> >("scenario");

        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
            << "C: 2 UID FETCH " + QByteArray::number(item1.id()) + " (UID COLLECTIONID)"
            << "S: * " + QByteArray::number(item1.id()) + " FETCH (UID " + QByteArray::number(item1.id()) + " REV 0 MIMETYPE \"" + item1.mimeType().name().toLatin1() + "\" COLLECTIONID " + QByteArray::number(col.id()) + ")"
            << "S: 2 OK UID FETCH completed";

            QTest::newRow("basic fetch") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
            << "C: 2 FETCH 1:* COLLECTIONID " + QByteArray::number(col.id()) + " (UID COLLECTIONID)"
            << "S: * " + QByteArray::number(item2.id()) + " FETCH (UID " + QByteArray::number(item2.id()) + " REV 0 MIMETYPE \"" + item2.mimeType().name().toLatin1() + "\" COLLECTIONID " + QByteArray::number(col.id()) + ")"
            << "S: * " + QByteArray::number(item1.id()) + " FETCH (UID " + QByteArray::number(item1.id()) + " REV 0 MIMETYPE \"" + item1.mimeType().name().toLatin1() + "\" COLLECTIONID " + QByteArray::number(col.id()) + ")";
            QTest::newRow("collection context") << scenario;
        }
    }

    void testFetch()
    {
        QFETCH(QList<QByteArray>, scenario);

        FakeAkonadiServer::instance()->setScenario(scenario);
        FakeAkonadiServer::instance()->runTest();
    }

    void testFetchByTag_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col = initializer->createCollection("root");
        PimItem item1 = initializer->createItem("item1", col);
        PimItem item2 = initializer->createItem("item2", col);

        Tag tag;
        TagType type;
        type.setName(QLatin1String("PLAIN"));
        type.insert();
        tag.setTagType(type);
        tag.setGid(QLatin1String("gid"));
        tag.insert();

        item1.addTag(tag);
        item1.update();

        QTest::addColumn<QList<QByteArray> >("scenario");

        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
            << "C: 2 UID FETCH 1:* TAGID " + QByteArray::number(tag.id()) + " (UID COLLECTIONID)"
            << "S: * " + QByteArray::number(item1.id()) + " FETCH (UID " + QByteArray::number(item1.id()) + " REV 0 MIMETYPE \"" + item1.mimeType().name().toLatin1() + "\" COLLECTIONID " + QByteArray::number(col.id()) + ")"
            << "S: 2 OK UID FETCH completed";

            QTest::newRow("fetch by tag") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
            << FakeAkonadiServer::selectResourceScenario(QLatin1String("testresource"))
            << "C: 2 UID FETCH 1:* TAGID " + QByteArray::number(tag.id()) + " (UID COLLECTIONID)"
            << "S: * " + QByteArray::number(item1.id()) + " FETCH (UID " + QByteArray::number(item1.id()) + " REV 0 MIMETYPE \"" + item1.mimeType().name().toLatin1() + "\" COLLECTIONID " + QByteArray::number(col.id()) + ")"
            << "S: 2 OK UID FETCH completed";

            QTest::newRow("uid fetch by tag from resource") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
            << FakeAkonadiServer::selectResourceScenario(QLatin1String("testresource"))
            << "C: 2 FETCH 1:* TAGID " + QByteArray::number(tag.id()) + " (UID COLLECTIONID)"
            << "S: * " + QByteArray::number(item1.id()) + " FETCH (UID " + QByteArray::number(item1.id()) + " REV 0 MIMETYPE \"" + item1.mimeType().name().toLatin1() + "\" COLLECTIONID " + QByteArray::number(col.id()) + ")"
            << "S: 2 OK FETCH completed";

            QTest::newRow("fetch by tag from resource") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
            << FakeAkonadiServer::selectResourceScenario(QLatin1String("testresource"))
            << "C: 2 FETCH 1:* COLLECTIONID " + QByteArray::number(col.id()) + " (UID COLLECTIONID)"
            << "S: * " + QByteArray::number(item2.id()) + " FETCH (UID " + QByteArray::number(item2.id()) + " REV 0 MIMETYPE \"" + item2.mimeType().name().toLatin1() + "\" COLLECTIONID " + QByteArray::number(col.id()) + ")"
            << "S: * " + QByteArray::number(item1.id()) + " FETCH (UID " + QByteArray::number(item1.id()) + " REV 0 MIMETYPE \"" + item1.mimeType().name().toLatin1() + "\" COLLECTIONID " + QByteArray::number(col.id()) + ")"
            << "S: 2 OK FETCH completed";

            QTest::newRow("fetch collection") << scenario;
        }
    }

    void testFetchByTag()
    {
        QFETCH(QList<QByteArray>, scenario);

        FakeAkonadiServer::instance()->setScenario(scenario);
        FakeAkonadiServer::instance()->runTest();
    }

    void testFetchCommandContext_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");
        PimItem item1 = initializer->createItem("item1", col1);
        Collection col2 = initializer->createCollection("col2");

        Tag tag;
        TagType type;
        type.setName(QLatin1String("PLAIN"));
        type.insert();
        tag.setTagType(type);
        tag.setGid(QLatin1String("gid"));
        tag.insert();

        item1.addTag(tag);
        item1.update();

        QTest::addColumn<QList<QByteArray> >("scenario");

        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
            << FakeAkonadiServer::selectResourceScenario(QLatin1String("testresource"))
            << "C: 2 FETCH 1:* COLLECTIONID " + QByteArray::number(col2.id()) + " (UID COLLECTIONID)"
            << "S: 2 OK FETCH completed"
            << "C: 3 FETCH 1:* TAGID " + QByteArray::number(tag.id()) + " (UID COLLECTIONID)"
            << "S: * " + QByteArray::number(item1.id()) + " FETCH (UID " + QByteArray::number(item1.id()) + " REV 0 MIMETYPE \"" + item1.mimeType().name().toLatin1() + "\" COLLECTIONID " + QByteArray::number(col1.id()) + ")"
            << "S: 3 OK FETCH completed";

            //Special case that used to be broken due to persistent command context
            QTest::newRow("fetch by tag after collection") << scenario;
        }
    }

    void testFetchCommandContext()
    {
        QFETCH(QList<QByteArray>, scenario);

        FakeAkonadiServer::instance()->setScenario(scenario);
        FakeAkonadiServer::instance()->runTest();
    }

    void testList_data()
    {
        QElapsedTimer timer;
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");

        timer.start();
        QList<PimItem> items;
        for (int i = 0; i< 1000; i++) {
            items.append(initializer->createItem(QString::number(i).toAscii().constData(),col1));
        }
        akDebug() << timer.nsecsElapsed()/1.0e6 << "ms";
        timer.start();

        QTest::addColumn<QList<QByteArray> >("scenario");

        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
            << "C: 2 FETCH 1:* COLLECTIONID " + QByteArray::number(col1.id()) + "CACHEONLY FULLPAYLOAD ALLATTR IGNOREERRORS ANCESTORS INF EXTERNALPAYLOAD (UID COLLECTIONID FLAGS SIZE REMOTEID REMOTEREVISION ATR:ENTITYDISPLAY)";
            while (!items.isEmpty()) {
                const PimItem &item = items.takeLast();
                scenario << "S: * " + QByteArray::number(item.id()) + " FETCH (UID " + QByteArray::number(item.id()) + " REV 0 REMOTEID \""+item.remoteId().toAscii()+"\" MIMETYPE \"test\" COLLECTIONID "+QByteArray::number(col1.id())+" SIZE 0 FLAGS () ANCESTORS (("+QByteArray::number(col1.id())+" \""+col1.name().toAscii()+"\") (0 \"\")))";
            }
            scenario << "S: 2 OK FETCH completed";
            QTest::newRow("complete list") << scenario;
        }
        akDebug() << timer.nsecsElapsed()/1.0e6 << "ms";
    }

    void testList()
    {
        QFETCH(QList<QByteArray>, scenario);

        FakeAkonadiServer::instance()->setScenario(scenario);
        //StorageDebugger::instance()->enableSQLDebugging(true);
        //StorageDebugger::instance()->writeToFile(QLatin1String("sqllog.txt"));
        FakeAkonadiServer::instance()->runTest();
    }

};

AKTEST_FAKESERVER_MAIN(FetchHandlerTest)

#include "fetchhandlertest.moc"
