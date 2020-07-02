/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <QObject>

#include "fakeakonadiserver.h"
#include "fakeconnection.h"
#include "aktest.h"
#include "entities.h"

#include <private/standarddirs_p.h>
#include <private/externalpartstorage_p.h>

#include "storage/partstreamer.h"
#include <storage/parthelper.h>

#include <QTest>
#include <QSettings>
#include <QDir>

#include <private/scope_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Akonadi::Server::PimItem)
Q_DECLARE_METATYPE(Akonadi::Server::Part::Storage)

class PartStreamerTest : public QObject
{
    Q_OBJECT

    FakeAkonadiServer mAkonadi;
public:
    PartStreamerTest()
    {
        // Set a very small treshold for easier testing
        const QString serverConfigFile = StandardDirs::serverConfigFile(StandardDirs::ReadWrite);
        QSettings settings(serverConfigFile, QSettings::IniFormat);
        settings.setValue(QStringLiteral("General/SizeThreshold"), 5);

        mAkonadi.init();
    }

    Protocol::ModifyItemsCommandPtr createCommand(const PimItem &item)
    {
        auto cmd = Protocol::ModifyItemsCommandPtr::create(item.id());
        cmd->setParts({ "PLD:DATA" });
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
        QTest::addColumn<Part::Storage>("storage");
        QTest::addColumn<PimItem>("pimItem");

        PimItem item;
        item.setCollectionId(Collection::retrieveByName(QStringLiteral("Col A")).id());
        item.setMimeType(MimeType::retrieveByName(QStringLiteral("application/octet-stream")));
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
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 3)))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", "123"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyItemsResponsePtr::create(item.id(), 1));

            QTest::newRow("item 1, internal") << scenarios << QByteArray("PLD:DATA") << QByteArray("123") << QByteArray() << 3ll << true << Part::Internal << item;
        }

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(item))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 9)))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data, QStringLiteral("%1_r0").arg(partId)))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyItemsResponsePtr::create(item.id(), 2));

            QTest::newRow("item 1, change to external") << scenarios << QByteArray("PLD:DATA") << QByteArray("15_r0") << QByteArray("123456789") << 9ll << true << Part::External << item;
        }

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(item))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 9)))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data, QStringLiteral("%1_r1").arg(partId)))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyItemsResponsePtr::create(item.id(), 3));

            QTest::newRow("item 1, update external") << scenarios << QByteArray("PLD:DATA") << QByteArray("15_r1") << QByteArray("987654321") << 9ll << true << Part::External << item;
        }

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(item))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 9)))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data, QStringLiteral("%1_r2").arg(partId)))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyItemsResponsePtr::create(item.id(), 4));

            QTest::newRow("item 1, external, no change") << scenarios << QByteArray("PLD:DATA") << QByteArray("15_r2") << QByteArray("987654321") << 9ll << false << Part::External << item;
        }

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(item))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 4)))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", "1234"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyItemsResponsePtr::create(item.id(), 5));

            QTest::newRow("item 1, change to internal") << scenarios << QByteArray("PLD:DATA") << QByteArray("1234") << QByteArray() << 4ll << true << Part::Internal << item;
        }

        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(item))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 4)))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", "1234"))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyItemsResponsePtr::create(item.id(), 6));

            QTest::newRow("item 1, internal, no change") << scenarios << QByteArray("PLD:DATA") << QByteArray("1234") << QByteArray() << 4ll << false << Part::Internal << item;
        }

        // Insert new item
        PimItem item2 = item;
        QVERIFY(item2.insert());

        const QString foreignPath = FakeAkonadiServer::basePath() + QStringLiteral("/tmp/foreignPayloadFile");
        {
            TestScenario::List scenarios;
            scenarios << FakeAkonadiServer::loginScenario()
                      << TestScenario::create(5, TestScenario::ClientCmd, createCommand(item2))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 3, 0, Protocol::PartMetaData::Foreign)))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
                      << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", foreignPath.toUtf8()))
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::ModifyItemsResponsePtr::create(item2.id(), 1));

            QTest::newRow("item 2, new foreign part") << scenarios << QByteArray("PLD:DATA") << foreignPath.toUtf8() << QByteArray("123") << 3ll << false << Part::Foreign << item2;
        }
    }

    void testStreamer()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(QByteArray, expectedPartName);
        QFETCH(QByteArray, expectedPartData);
        QFETCH(QByteArray, expectedFileData);
        QFETCH(qint64, expectedPartSize);
        QFETCH(Part::Storage, storage);
        QFETCH(PimItem, pimItem);

        if (storage == Part::External) {
            // Create the payload file now, since don't have means to react
            // directly to the streaming command
            QFile file(ExternalPartStorage::resolveAbsolutePath(expectedPartData));
            file.open(QIODevice::WriteOnly);
            file.write(expectedFileData);
            file.close();
        } else if (storage == Part::Foreign) {
            // Create the foreign payload file
            QDir().mkpath(FakeAkonadiServer::basePath() + QStringLiteral("/tmp"));
            QFile file(QString::fromUtf8(expectedPartData));
            file.open(QIODevice::WriteOnly);
            file.write(expectedFileData);
            file.close();
        }

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();

        PimItem item = PimItem::retrieveById(pimItem.id());
        const QVector<Part> parts = item.parts();
        QVERIFY(parts.count() == 1);
        const Part part = parts[0];
        QCOMPARE(part.datasize(), expectedPartSize);
        QCOMPARE(part.storage(), storage);
        const QByteArray data = part.data();

        if (storage == Part::External) {
            QCOMPARE(data, expectedPartData);
            QFile file(ExternalPartStorage::resolveAbsolutePath(data));
            QVERIFY(file.exists());
            QCOMPARE(file.size(), expectedPartSize);
            QVERIFY(file.open(QIODevice::ReadOnly));

            const QByteArray fileData = file.readAll();
            QCOMPARE(fileData, expectedFileData);

            // Make sure no previous versions are left behind in file_db_data
            const int revision = data.mid(data.indexOf("_r") + 2).toInt();
            for (int i = 0; i < revision; ++i) {
                const QByteArray fileName = QByteArray::number(part.id()) + "_r" + QByteArray::number(i);
                const QString filePath = ExternalPartStorage::resolveAbsolutePath(fileName);
                // TRY because the deletion happens in another thread
                QTRY_VERIFY2(!QFile::exists(filePath), qPrintable(filePath));
            }
        } else if (storage == Part::Foreign) {
            QCOMPARE(data, expectedPartData);
            QFile file(QString::fromUtf8(data));
            QVERIFY(file.exists());
            QCOMPARE(file.size(), expectedPartSize);
            QVERIFY(file.open(QIODevice::ReadOnly));

            const QByteArray fileData = file.readAll();
            QCOMPARE(fileData, expectedFileData);
        } else {
            QCOMPARE(data, expectedPartData);

            // Make sure nothing is left behind in file_db_data
            // TODO: we have no way of knowing what is the last revision
            for (int i = 0; i <= 100; ++i) {
                const QByteArray fileName = QByteArray::number(part.id()) + "_r" + QByteArray::number(i);
                const QString filePath = ExternalPartStorage::resolveAbsolutePath(fileName);
                QTRY_VERIFY2(!QFile::exists(filePath), qPrintable(filePath));
            }
        }
    }

};

AKTEST_FAKESERVER_MAIN(PartStreamerTest)

#include "partstreamertest.moc"
