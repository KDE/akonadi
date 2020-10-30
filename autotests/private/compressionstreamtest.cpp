/*
    SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "compressionstream_p.h"

#include <QObject>
#include <QTest>
#include <QBuffer>
#include <QRandomGenerator>

#include <array>

using namespace Akonadi;

class CompressionStreamTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCompression_data()
    {
        QTest::addColumn<QByteArray>("testData");

        QTest::newRow("Null") << QByteArray{};
        QTest::newRow("Empty") << QByteArray("");
        QTest::newRow("Hello world") << QByteArray("Hello world");
    }

    void testCompression()
    {
        QFETCH(QByteArray, testData);

        QByteArray compressedData;
        QBuffer compressedBuffer(&compressedData);
        compressedBuffer.open(QIODevice::WriteOnly);

        {
            CompressionStream stream(&compressedBuffer);
            QVERIFY(stream.open(QIODevice::WriteOnly));
            QCOMPARE(stream.write(testData), testData.size());
            stream.close();
            QVERIFY(!stream.error());
        }

        compressedBuffer.close();
        compressedBuffer.open(QIODevice::ReadOnly);
        QByteArray decompressedData;

        {
            CompressionStream stream(&compressedBuffer);
            QVERIFY(stream.open(QIODevice::ReadOnly));
            decompressedData = stream.readAll();
            stream.close();
            QVERIFY(!stream.error());
        }

        QCOMPARE(decompressedData.size(), testData.size());
        QCOMPARE(decompressedData, testData);
    }

    void testUnbufferedCompressionOfLargeText()
    {
        std::array<std::string, 101> loremIpsum = {
            "Lorem", "ipsum", "dolor", "sit", "amet,", "consectetur", "adipiscing", "elit.", "Integer",
            "dictum", "massa", "orci,", "eget", "tempor", "neque", "euismod", "a.", "Suspendisse", "mi",
            "arcu,", "facilisis", "eu", "risus", "at,", "varius", "vehicula", "mi.", "Proin", "tristique",
            "eros", "nisl,", "vel", "porttitor", "erat", "elementum", "at.", "Quisque", "et", "ex", "id",
            "risus", "hendrerit", "rhoncus", "eu", "vel", "enim.", "Vivamus", "at", "lorem", "laoreet", "ex",
            "mattis", "feugiat", "vitae", "sit", "amet", "sem.", "Vestibulum", "in", "ante", "sagittis,",
            "venenatis", "nibh", "et,", "consectetur", "est.", "Donec", "cursus", "enim", "ac", "pellentesque",
            "euismod.", "Nullam", "interdum", "metus", "sed", "blandit", "dapibus.", "Ut", "nec", "euismod",
            "magna.", "Aenean", "gravida", "elit", "metus,", "eget", "vehicula", "nibh", "euismod", "ut.",
            "Vestibulum", "risus", "lectus,", "molestie", "elementum", "lobortis", "at,", "finibus", "a", "quam."
        };

        QByteArray testData;
        QRandomGenerator *generator = QRandomGenerator::system();
        while (testData.size() < 10 * 1024) {
            testData += QByteArray(" ") + loremIpsum[generator->bounded(100)].c_str();
        }

        QByteArray compressedData;
        QBuffer compressedBuffer(&compressedData);
        compressedBuffer.open(QIODevice::WriteOnly);

        {
            CompressionStream stream(&compressedBuffer);
            QVERIFY(stream.open(QIODevice::WriteOnly | QIODevice::Unbuffered));
            qint64 written = 0;
            for (int i = 0; i < testData.size(); ++i) {
                written += stream.write(testData.constData() + i, 1);
            }
            QCOMPARE(written, testData.size());
            stream.close();
            QVERIFY(!stream.error());
        }

        compressedBuffer.close();
        compressedBuffer.open(QIODevice::ReadOnly);

        QByteArray decompressedData;
        {
            CompressionStream stream(&compressedBuffer);
            QVERIFY(stream.open(QIODevice::ReadOnly | QIODevice::Unbuffered));
            while (!stream.atEnd()) {
                char buf[3] = {};
                const auto read = stream.read(buf, sizeof(buf));
                decompressedData.append(buf, read);
            }
            stream.close();
            QVERIFY(!stream.error());
        }

        QCOMPARE(decompressedData.size(), testData.size());
        QCOMPARE(decompressedData, testData);
    }

    void testDetection_data()
    {
        QTest::addColumn<QVector<uint8_t>>("data");
        QTest::addColumn<bool>("isValid");

        QTest::newRow("Too short - random") << QVector<uint8_t>{0x65, 0x66} << false;
        QTest::newRow("Too short - valid start") << QVector<uint8_t>{0xfd, 0x37, 0x7a, 0x58, 0x5a} << false;
        QTest::newRow("Valid magic only") << QVector<uint8_t>{0xfd, 0x37, 0x7a, 0x58, 0x5a, 0x00} << true;
        QTest::newRow("Valid input") << QVector<uint8_t>{
            0xfd, 0x37, 0x7a, 0x58, 0x5a, 0x00, 0x00, 0x04, 0xe6, 0xd6,
            0xb4, 0x46, 0x02, 0x00, 0x21, 0x01, 0x16, 0x00, 0x00, 0x00,
            0x74, 0x2f, 0xe5, 0xa3, 0x01, 0x00, 0x01, 0x41, 0x0a, 0x00,
            0x00, 0x00, 0x8f, 0xe8, 0x69, 0xe6, 0x2b, 0x6a, 0xcd, 0x94,
            0x00, 0x01, 0x1a, 0x02, 0xdc, 0x2e, 0xa5, 0x7e, 0x1f, 0xb6,
            0xf3, 0x7d, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0x59, 0x5a } << true;
    }

    void testDetection()
    {
        QFETCH(QVector<uint8_t>, data);
        QFETCH(bool, isValid);

        QByteArray ba(reinterpret_cast<const char *>(data.constData()), data.size());
        QBuffer buffer(&ba);
        QVERIFY(buffer.open(QIODevice::ReadOnly));

        QCOMPARE(CompressionStream::isCompressed(&buffer), isValid);
        QCOMPARE(buffer.pos(), 0);
    }
};

QTEST_GUILESS_MAIN(CompressionStreamTest)

#include "compressionstreamtest.moc"
