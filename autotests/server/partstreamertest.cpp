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

#include <QObject>

#include "fakeakonadiserver.h"
#include "fakeconnection.h"
#include "akstandarddirs.h"
#include "akdebug.h"
#include "aktest.h"
#include "entities.h"

#include "storage/partstreamer.h"
#include <storage/parthelper.h>

#include <QtTest>
#include <QSettings>

#include <private/scope_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Akonadi::Server::PimItem)
Q_DECLARE_METATYPE(Akonadi::Protocol::Command)


class PartStreamerTest : public QObject
{
    Q_OBJECT

public:
    PartStreamerTest()
    {
        // Set a very small treshold for easier testing
        const QString serverConfigFile = AkStandardDirs::serverConfigFile(XdgBaseDirs::ReadWrite);
        QSettings settings(serverConfigFile, QSettings::IniFormat);
        settings.setValue(QLatin1String("General/SizeThreshold"), 5);

        try {
            FakeAkonadiServer::instance()->init();
        } catch (const FakeAkonadiServerException &e) {
            akError() << "Server exception: " << e.what();
            akFatal() << "Fake Akonadi Server failed to start up, aborting test";
        }
    }

    ~PartStreamerTest()
    {
        FakeAkonadiServer::instance()->quit();
    }

    Protocol::ModifyItemsCommand createCommand(const PimItem &item)
    {
        Protocol::ModifyItemsCommand cmd(item.id());
        cmd.setParts({ "PLD:DATA" });
        return cmd;
    }

private Q_SLOTS:
    void testStreamer_data()
    {
        QTest::addColumn<TestScenario::List>("scenarios");
        QTest::addColumn<QByteArray>("expectedPartName");
        QTest::addColumn<QByteArray>("expectedPartData");
        QTest::addColumn<QByteArray>("expectedFileData");
        QTest::addColumn<qint64>("expectedPartSize");
        QTest::addColumn<bool>("expectedChanged");
        QTest::addColumn<bool>("isExternal");
        QTest::addColumn<PimItem>("pimItem");

        PimItem item;
        item.setCollectionId(Collection::retrieveByName(QLatin1String("Col A")).id());
        item.setMimeType(MimeType::retrieveByName(QLatin1String("application/octet-stream")));
        item.setSize(1); // this will not match reality during the test, but that does not matter, as
        // that's not the subject of this test
        QVERIFY(item.insert());

        qint64 partId = -1;
        Part::List parts = Part::retrieveAll();
        if (parts.isEmpty()) {
            partId = 0;
        } else {
            partId = parts.last().id() + 1;
        }

        // Order of these tests matters!
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(item))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommand("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 3)))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommand("PLD:DATA", Protocol::StreamPayloadCommand::Data))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponse("PLD:DATA", "123"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyItemsResponse(item.id(), 1));

            QTest::newRow("item 1, internal") << scenarios << QByteArray("PLD:DATA") << QByteArray("123") << QByteArray() << 3ll << true << false << item;
        }

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(item))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommand("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 9)))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommand("PLD:DATA", Protocol::StreamPayloadCommand::Data, QString::fromLatin1("%1_r0").arg(partId)))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponse("PLD:DATA"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyItemsResponse(item.id(), 2));

            QTest::newRow("item 1, change to external") << scenarios << QByteArray("PLD:DATA") << QByteArray("15_r0") << QByteArray("123456789") << 9ll << true << true << item;
        }

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(item))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommand("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 9)))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommand("PLD:DATA", Protocol::StreamPayloadCommand::Data, QString::fromLatin1("%1_r1").arg(partId)))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponse("PLD:DATA"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyItemsResponse(item.id(), 3));

            QTest::newRow("item 1, update external") << scenarios << QByteArray("PLD:DATA") << QByteArray("15_r1") << QByteArray("987654321") << 9ll << true << true << item;
        }

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(item))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommand("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 9)))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommand("PLD:DATA", Protocol::StreamPayloadCommand::Data, QString::fromLatin1("%1_r2").arg(partId)))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponse("PLD:DATA"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyItemsResponse(item.id(), 4));

            QTest::newRow("item 1, external, no change") << scenarios << QByteArray("PLD:DATA") << QByteArray("15_r2") << QByteArray("987654321") << 9ll << false << true << item;
        }

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(item))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommand("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 4)))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommand("PLD:DATA", Protocol::StreamPayloadCommand::Data))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponse("PLD:DATA", "1234"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyItemsResponse(item.id(), 5));

            QTest::newRow("item 1, change to internal") << scenarios << QByteArray("PLD:DATA") << QByteArray("1234") << QByteArray() << 4ll << true << false << item;
        }

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(item))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommand("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 4)))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommand("PLD:DATA", Protocol::StreamPayloadCommand::Data))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponse("PLD:DATA", "1234"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyItemsResponse(item.id(), 6));

            QTest::newRow("item 1, internal, no change") << scenarios << QByteArray("PLD:DATA") << QByteArray("1234") << QByteArray() << 4ll << false << false << item;
        }
    }

    void testStreamer()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(QByteArray, expectedPartName);
        QFETCH(QByteArray, expectedPartData);
        QFETCH(QByteArray, expectedFileData);
        QFETCH(qint64, expectedPartSize);
        QFETCH(bool, isExternal);
        QFETCH(PimItem, pimItem);

        if (isExternal) {
            // Create the payload file now, since don't have means to react
            // directly to the streaming command
            QFile file(PartHelper::resolveAbsolutePath(expectedPartData));
            file.open(QIODevice::WriteOnly);
            file.write(expectedFileData);
            file.close();
        }

        FakeAkonadiServer::instance()->setScenarios(scenarios);
        FakeAkonadiServer::instance()->runTest();

        PimItem item = PimItem::retrieveById(pimItem.id());
        const QVector<Part> parts = item.parts();
        QVERIFY(parts.count() == 1);
        const Part part = parts[0];
        QCOMPARE(part.datasize(), expectedPartSize);
        QCOMPARE(part.external(), isExternal);
        const QByteArray data = part.data();

        if (isExternal) {
            QCOMPARE(data, expectedPartData);
            QFile file(PartHelper::resolveAbsolutePath(data));
            QVERIFY(file.exists());
            QCOMPARE(file.size(), expectedPartSize);
            QVERIFY(file.open(QIODevice::ReadOnly));

            const QByteArray fileData = file.readAll();
            QCOMPARE(fileData, expectedFileData);

            // Make sure no previous versions are left behind in file_db_data
            for (int i = 0; i < part.version(); ++i) {
                const QByteArray fileName = QByteArray::number(part.id()) + "_r" + QByteArray::number(part.version());
                const QString filePath = PartHelper::resolveAbsolutePath(fileName);
                QVERIFY(!QFile::exists(filePath));
            }
        } else {
            QCOMPARE(data, expectedPartData);

            // Make sure nothing is left behind in file_db_data
            for (int i = 0; i <= part.version(); ++i) {
                const QByteArray fileName = QByteArray::number(part.id()) + "_r" + QByteArray::number(part.version());
                const QString filePath = PartHelper::resolveAbsolutePath(fileName);
                QVERIFY(!QFile::exists(filePath));
            }
        }
    }

};

AKTEST_FAKESERVER_MAIN(PartStreamerTest)

#include "partstreamertest.moc"
