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
#include "imapstreamparser.h"
#include "entities.h"

#include "storage/partstreamer.h"
#include <storage/parthelper.h>

#include <QtTest>
#include <QSettings>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Akonadi::Server::PimItem)

class PartStreamerTest : public QObject
{
    Q_OBJECT

public:
    PartStreamerTest()
    {
        qRegisterMetaType<Akonadi::Server::Response>();

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


private Q_SLOTS:
    void slotStreamerResponseAvailable(const Akonadi::Server::Response &response)
    {
        const QByteArray string = response.asString();
        const qint64 fnStart = string.indexOf("[FILE ") + 6;
        const qint64 fnEnd = string.indexOf("]", fnStart);
        const QByteArray name = string.mid(fnStart, fnEnd - fnStart);
        const QString fileName = PartHelper::resolveAbsolutePath(name);
        qDebug() << string << fileName;
        QFile f(fileName);
        QVERIFY(!f.exists());
        QVERIFY(f.open(QIODevice::ReadWrite));

        QFETCH(QByteArray, expectedData);
        qDebug() << "Wrote" << f.write(expectedData) << "bytes to" << f.fileName();
        f.close();
    }

    void testStreamer_data()
    {
        QTest::addColumn<QByteArray>("expectedPartName");
        QTest::addColumn<QByteArray>("expectedData");
        QTest::addColumn<qint64>("expectedPartSize");
        QTest::addColumn<bool>("expectedChanged");
        QTest::addColumn<bool>("isExternal");
        QTest::addColumn<int>("version");
        QTest::addColumn<PimItem>("pimItem");

        PimItem item;
        item.setCollectionId(Collection::retrieveByName(QLatin1String("Col A")).id());
        item.setMimeType(MimeType::retrieveByName(QLatin1String("application/octet-stream")));
        item.setSize(1); // this will not match reality during the test, but that does not matter, as
                         // that's not the subject of this test
        QVERIFY(item.insert());

        // Order of these tests matters!
        QTest::newRow("item 1, internal") << QByteArray("PLD:DATA") << QByteArray("123") << 3ll << true << false << -1 << item;
        QTest::newRow("item 1, change to external") << QByteArray("PLD:DATA") << QByteArray("123456789") << 9ll << true << true << 0 << item;
        QTest::newRow("item 1, update external") << QByteArray("PLD:DATA") << QByteArray("987654321") << 9ll << true << true << 1 << item;
        QTest::newRow("item 1, external, no change") << QByteArray("PLD:DATA") << QByteArray("987654321") << 9ll << false << true << 2 << item;
        QTest::newRow("item 1, change to internal") << QByteArray("PLD:DATA") << QByteArray("1234") << 4ll << true << false << 2 << item;
        QTest::newRow("item 1, internal, no change") << QByteArray("PLD:DATA") << QByteArray("1234") << 4ll << false << false << 2 << item;
    }

    void testStreamer()
    {
        QFETCH(QByteArray, expectedPartName);
        QFETCH(QByteArray, expectedData);
        QFETCH(qint64, expectedPartSize);
        QFETCH(bool, expectedChanged);
        QFETCH(bool, isExternal);
        QFETCH(int, version);
        QFETCH(PimItem, pimItem);

        FakeConnection connection;
        ClientCapabilities capabilities;
        capabilities.setAkAppendStreaming(true);
        capabilities.setDirectStreaming(true);
        capabilities.setNoPayloadPath(true);
        connection.setCapabilities(capabilities);

        QBuffer buffer;
        QVERIFY(buffer.open(QIODevice::ReadWrite));
        buffer.write("PLD:DATA[0] {" + QByteArray::number(expectedPartSize) + "}\n");
        if (!isExternal) {
            buffer.write(expectedData);
        } else {
            buffer.write(")\n");
        }
        buffer.seek(0);
        ImapStreamParser parser(&buffer);
        const QByteArray command = parser.readString();

        PartStreamer streamer(&connection, &parser, pimItem);
        connect(&streamer, SIGNAL(responseAvailable(Akonadi::Server::Response)),
                this, SLOT(slotStreamerResponseAvailable(Akonadi::Server::Response)));
        QSignalSpy streamerSpy(&streamer, SIGNAL(responseAvailable(Akonadi::Server::Response)));

        QByteArray partName;
        qint64 partSize;
        bool changed = false;

        try {
            QVERIFY(streamer.stream(command, false, partName, partSize, &changed));
        } catch (const Exception &e) {
            qDebug() << e.type() << ":" << e.what();
            QFAIL("Caught an unexpected exception");
        }

        QCOMPARE(QString::fromUtf8(partName), QString::fromUtf8(expectedPartName));
        QCOMPARE(partSize, expectedPartSize);
        QCOMPARE(expectedChanged, changed);

        PimItem item = PimItem::retrieveById(pimItem.id());
        const QVector<Part> parts = item.parts();
        QCOMPARE(parts.count(), 1);
        const Part part = parts[0];
        QCOMPARE(part.datasize(), expectedPartSize);
        QCOMPARE(part.external(), isExternal);
        qDebug() << part.version() << part.data();
        const QByteArray data = part.data();
        if (isExternal) {
            QVERIFY(streamerSpy.count() == 1);
            QVERIFY(streamerSpy.first().count() == 1);
            const Response response = streamerSpy.first().first().value<Akonadi::Server::Response>();
            const QByteArray str = response.asString();
            const QByteArray expectedResponse = "+ STREAM [FILE " + QByteArray::number(part.id()) + "_r" + QByteArray::number(version) + "]";
            QCOMPARE(QString::fromUtf8(str), QString::fromUtf8(expectedResponse));

            QFile file(PartHelper::resolveAbsolutePath(data));
            QVERIFY(file.exists());
            QCOMPARE(file.size(), expectedPartSize);
            QVERIFY(file.open(QIODevice::ReadOnly));
            const QByteArray fileData = file.readAll();
            QCOMPARE(QString::fromUtf8(fileData), QString::fromUtf8(expectedData));
            QCOMPARE(fileData, expectedData);

            // Make sure no previous versions are left behind in file_db_data
            for (int i = 0; i < version; ++i) {
                const QByteArray fileName = QByteArray::number(part.id()) + "_r" + QByteArray::number(part.version());
                const QString filePath = PartHelper::resolveAbsolutePath(fileName);
                QVERIFY(!QFile::exists(filePath));
            }
        } else {
            QVERIFY(streamerSpy.isEmpty());

            QCOMPARE(QString::fromUtf8(data), QString::fromUtf8(expectedData));
            QCOMPARE(data, expectedData);

            // Make sure nothing is left behind in file_db_data
            for (int i = 0; i <= version; ++i) {
                const QByteArray fileName = QByteArray::number(part.id()) + "_r" + QByteArray::number(part.version());
                const QString filePath = PartHelper::resolveAbsolutePath(fileName);
                QVERIFY(!QFile::exists(filePath));
            }
        }
    }

};

AKTEST_FAKESERVER_MAIN(PartStreamerTest)

#include "partstreamertest.moc"
