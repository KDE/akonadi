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
#include <handler/create.h>
#include <imapstreamparser.h>
#include <response.h>
#include <storage/entity.h>

#include "fakeakonadiserver.h"
#include "aktest.h"
#include "akdebug.h"
#include "entities.h"

#include <QtTest/QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

class CreateHandlerTest : public QObject
{
    Q_OBJECT

public:
    CreateHandlerTest()
    {
        qRegisterMetaType<Akonadi::Server::Response>();

        try {
            FakeAkonadiServer::instance()->init();
        } catch (const FakeAkonadiServerException &e) {
            akError() << "Server exception: " << e.what();
            akFatal() << "Fake Akonadi Server failed to start up, aborting test";
        }
    }

    ~CreateHandlerTest()
    {
        FakeAkonadiServer::instance()->quit();
    }

private Q_SLOTS:
    void testCreate_data()
    {
        QTest::addColumn<QList<QByteArray> >("scenario");
        QTest::addColumn<Akonadi::NotificationMessageV3>("notification");

        Akonadi::NotificationMessageV3 notificationTemplate;
        notificationTemplate.setType(NotificationMessageV2::Collections);
        notificationTemplate.setOperation(NotificationMessageV2::Add);
        notificationTemplate.setParentCollection(3);
        notificationTemplate.setResource("akonadi_fake_resource_0");
        notificationTemplate.setSessionId(FakeAkonadiServer::instanceName().toLatin1());

        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 CREATE \"New Name\" 3 (MYRANDOMATTRIBUTE \"\")"
                     << "S: * 8 3 (NAME \"New Name\" MIMETYPE (application/octet-stream inode/directory) REMOTEID \"\" REMOTEREVISION \"\" RESOURCE \"akonadi_fake_resource_0\" VIRTUAL 0 CACHEPOLICY (INHERIT true INTERVAL -1 CACHETIMEOUT -1 SYNCONDEMAND false LOCALPARTS (ALL)) ENABLED TRUE DISPLAY DEFAULT SYNC DEFAULT INDEX DEFAULT MYRANDOMATTRIBUTE \"\")"
                     << "S: 2 OK CREATE completed";

            Akonadi::NotificationMessageV3 notification = notificationTemplate;
            notification.addEntity(8, QLatin1String(""), QLatin1String(""));

            QTest::newRow("create collection") << scenario <<  notification;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 CREATE \"Name2\" 3 (ENABLED FALSE DISPLAY TRUE SYNC TRUE INDEX TRUE)"
                     << "S: * 9 3 (NAME \"Name2\" MIMETYPE (application/octet-stream inode/directory) REMOTEID \"\" REMOTEREVISION \"\" RESOURCE \"akonadi_fake_resource_0\" VIRTUAL 0 CACHEPOLICY (INHERIT true INTERVAL -1 CACHETIMEOUT -1 SYNCONDEMAND false LOCALPARTS (ALL)) ENABLED FALSE DISPLAY TRUE SYNC TRUE INDEX TRUE )"
                     << "S: 2 OK CREATE completed";

            Akonadi::NotificationMessageV3 notification = notificationTemplate;
            notification.addEntity(9, QLatin1String(""), QLatin1String(""));

            QTest::newRow("create collection with local override") << scenario <<  notification;
        }
    }

    void testCreate()
    {
        QFETCH(QList<QByteArray>, scenario);
        QFETCH(NotificationMessageV3, notification);

        FakeAkonadiServer::instance()->setScenario(scenario);
        FakeAkonadiServer::instance()->runTest();

        QSignalSpy *notificationSpy = FakeAkonadiServer::instance()->notificationSpy();
        if (notification.isValid()) {
            QCOMPARE(notificationSpy->count(), 1);
            const NotificationMessageV3::List notifications = notificationSpy->takeFirst().first().value<NotificationMessageV3::List>();
            QCOMPARE(notifications.count(), 1);
            QCOMPARE(notifications.first(), notification);
        } else {
            QVERIFY(notificationSpy->isEmpty() || notificationSpy->takeFirst().first().value<NotificationMessageV3::List>().isEmpty());
        }
    }

};

AKTEST_FAKESERVER_MAIN(CreateHandlerTest)

#include "createhandlertest.moc"
