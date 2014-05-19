/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#include <handler/akappend.h>
#include <imapstreamparser.h>
#include <response.h>

#include <libs/notificationmessagev3_p.h>
#include <libs/notificationmessagev2_p.h>
#include <imapparser_p.h>

#include "fakeakonadiserver.h"
#include "aktest.h"

#include <QtTest/QTest>
#include <QSignalSpy>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(PimItem);
Q_DECLARE_METATYPE(QVector<Part>);

class AkAppendHandlerTest : public QObject
{
    Q_OBJECT

private:
    FakeAkonadiServer mServer;

public:
    AkAppendHandlerTest()
    {
        qRegisterMetaType<Akonadi::Server::Response>();

        try {
            mServer.initialize();
        } catch (FakeAkonadiServerException &e) {
            akError() << e.what();
            akFatal() << "Fake Akonadi Server failed to start up, aborting test";
        }
    }

    ~AkAppendHandlerTest()
    {
    }


private Q_SLOTS:
    void testAkAppend_data()
    {
        QTest::addColumn<QList<QByteArray> >("scenario");
        QTest::addColumn<NotificationMessageV3>("notification");
        QTest::addColumn<PimItem>("pimItem");
        QTest::addColumn<QVector<Part> >("parts");
        QTest::addColumn<qint64>("uidnext");
        QTest::addColumn<QDateTime>("datetime");
        QTest::addColumn<bool>("expectFail");

        QList<QByteArray> scenario;
        NotificationMessageV3 notification;
        qint64 uidnext = 0;
        QDateTime datetime(QDate(2014, 05, 12), QTime(14, 46, 00), Qt::UTC);
        PimItem pimItem;
        QVector<Part> parts;

        pimItem.setCollectionId(4);
        pimItem.setSize(10);
        pimItem.setRemoteId(QLatin1String("TEST-1"));
        pimItem.setRemoteRevision(QLatin1String("1"));
        pimItem.setGid(QLatin1String("TEST-1"));
        pimItem.setMimeType(MimeType::retrieveByName(QLatin1String("application/octet-stream")));
        pimItem.setDatetime(datetime);
        Part part;
        part.setPartType(PartType::retrieveByName(QLatin1String("DATA")));
        part.setData("0123456789");
        part.setExternal(false);
        part.setDatasize(10);
        parts << part;
        notification.setType(NotificationMessageV2::Items);
        notification.setOperation(NotificationMessageV2::Add);
        notification.setParentCollection(4);
        notification.setResource("akonadi_fake_resource_0");
        notification.addEntity(-1, QLatin1String("TEST-1"), QLatin1String("1"), QLatin1String("application/octet-stream"));
        notification.setSessionId(FakeAkonadiServer::instanceName().toLatin1());
        uidnext = 13;
        scenario << FakeAkonadiServer::defaultScenario()
                 << "C: 2 X-AKAPPEND 4 10 (\\RemoteId[TEST-1] \\MimeType[application/octet-stream] \\RemoteRevision[1] \\Gid[TEST-1]) \"12-May-2014 14:46:00 +0000\" (PLD:DATA[0] {10}"
                 << "S: + Ready for literal data (expecting 10 bytes)"
                 << "C: 0123456789)"
                 << "S: 2 [UIDNEXT 13 DATETIME \"12-May-2014 14:46:00 +0000\"]"
                 << "S: 2 OK Append completed";
        QTest::newRow("single-part") << scenario << notification << pimItem << parts << uidnext << datetime << false;
    }

    void testAkAppend()
    {
        QFETCH(QList<QByteArray>, scenario);
        QFETCH(NotificationMessageV3, notification);
        QFETCH(PimItem, pimItem);
        QFETCH(QVector<Part>, parts);
        QFETCH(qint64, uidnext);
        QFETCH(bool, expectFail);

        mServer.setScenario(scenario);
        mServer.runTest();

        QSignalSpy *notificationSpy = mServer.notificationSpy();

        if (notification.isValid()) {
            QCOMPARE(notificationSpy->count(), 1);
            const NotificationMessageV3::List notifications = notificationSpy->at(0).first().value<NotificationMessageV3::List>();
            QCOMPARE(notifications.count(), 1);
            const NotificationMessageV3 itemNotification = notifications.first();

            QVERIFY(AkTest::compareNotifications(itemNotification, notification, QFlag(AkTest::NtfAll &~ AkTest::NtfEntities)));
            QCOMPARE(itemNotification.entities().count(), notification.entities().count());
        } else {
            QVERIFY(notificationSpy->isEmpty() || notificationSpy->takeFirst().first().value<NotificationMessageV3::List>().isEmpty());
        }

        const PimItem actualItem = PimItem::retrieveById(uidnext);
        if (expectFail) {
            QVERIFY(!actualItem.isValid());
        } else {
            QVERIFY(actualItem.isValid());
            const QList<Part> actualParts = Part::retrieveFiltered(Part::pimItemIdColumn(), actualItem.id()).toList();
            QCOMPARE(actualParts.count(), parts.count());
            Q_FOREACH (const Part &part, parts) {
                const QList<Part>::const_iterator actualPartIter =
                    std::find_if(actualParts.constBegin(), actualParts.constEnd(),
                                 [part] (Part const &actualPart) { return part.partTypeId() == actualPart.partTypeId(); });

                QVERIFY(actualPartIter != actualParts.constEnd());
                const Part actualPart = *actualPartIter;
                QVERIFY(actualPart.isValid());
                QCOMPARE(QString::fromUtf8(actualPart.data()), QString::fromUtf8(part.data()));
                QCOMPARE(actualPart.data(), part.data());
                QCOMPARE(actualPart.datasize(), part.datasize());
                QCOMPARE(actualPart.external(), part.external());
            }
        }
    }
};

AKTEST_FAKESERVER_MAIN(AkAppendHandlerTest)

#include "akappendhandlertest.moc"
