/*
    SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "datastream_p_p.h"

#include <QBuffer>
#include <QByteArray>
#include <QObject>
#include <QTest>
#include <qcontainerfwd.h>

enum TestEnum {
    Value1 = 1,
    Value2 = 2,
    Value3 = 3
};

enum class TestEnumClass : qint64 {
    Value1 = 1,
    Value2 = 2,
    Value3 = 3
};

class DataStreamTest : public QObject
{
    Q_OBJECT

private:
    template<typename T>
    void testIntegralType_data()
    {
        QTest::addColumn<T>("input");
        QTest::newRow("zero") << T(0);
        QTest::newRow("positive") << T(42);
        if constexpr (std::is_signed_v<T>) {
            QTest::newRow("negative") << T(-42);
        }
        QTest::newRow("max") << std::numeric_limits<T>::max();
        QTest::newRow("lowest") << std::numeric_limits<T>::lowest();
    }

    template<typename T>
    void testTypeStreaming()
    {
        QFETCH(T, input);

        QByteArray data;
        {
            QBuffer buffer(&data);
            buffer.open(QIODevice::WriteOnly);
            Akonadi::Protocol::DataStream stream(&buffer);

            stream << input;
            stream.flush();
        }

        T output;
        {
            QBuffer buffer(&data);
            buffer.open(QIODevice::ReadOnly);
            Akonadi::Protocol::DataStream stream(&buffer);

            stream >> output;
            buffer.close();
        }

        QCOMPARE(input, output);
    }

private Q_SLOTS:
    void testQInt8_data()
    {
        testIntegralType_data<qint8>();
    }
    void testQInt8()
    {
        testTypeStreaming<qint8>();
    }
    void testQUInt8_data()
    {
        testIntegralType_data<quint8>();
    }
    void testQUInt8()
    {
        testTypeStreaming<quint8>();
    }
    void testQInt16_data()
    {
        testIntegralType_data<qint16>();
    }
    void testQInt16()
    {
        testTypeStreaming<qint16>();
    }
    void testQUInt16_data()
    {
        testIntegralType_data<quint16>();
    }
    void testQUInt16()
    {
        testTypeStreaming<quint16>();
    }
    void testQInt32_data()
    {
        testIntegralType_data<qint32>();
    }
    void testQInt32()
    {
        testTypeStreaming<qint32>();
    }
    void testQUInt32_data()
    {
        testIntegralType_data<quint32>();
    }
    void testQUInt32()
    {
        testTypeStreaming<quint32>();
    }
    void testQInt64_data()
    {
        testIntegralType_data<qint64>();
    }
    void testQInt64()
    {
        testTypeStreaming<qint64>();
    }
    void testQUInt64_data()
    {
        testIntegralType_data<quint64>();
    }
    void testQUInt64()
    {
        testTypeStreaming<quint64>();
    }

    void testEnum_data()
    {
        QTest::addColumn<TestEnum>("input");
        QTest::newRow("Value1") << TestEnum::Value1;
        QTest::newRow("Value2") << TestEnum::Value2;
        QTest::newRow("Value3") << TestEnum::Value3;
    }

    void testEnum()
    {
        testTypeStreaming<TestEnum>();
    }

    void testEnumClass_data()
    {
        QTest::addColumn<TestEnumClass>("input");
        QTest::newRow("Value1") << TestEnumClass::Value1;
        QTest::newRow("Value2") << TestEnumClass::Value2;
        QTest::newRow("Value3") << TestEnumClass::Value3;
    }

    void testEnumClass()
    {
        testTypeStreaming<TestEnumClass>();
    }

    void testQString_data()
    {
        QTest::addColumn<QString>("input");
        QTest::newRow("null string") << QString();
        QTest::newRow("empty string") << QStringLiteral("");
        QTest::newRow("short string") << QStringLiteral("Hello World");
        QTest::newRow("utf8 string") << QStringLiteral("Hello 🌍");
    }

    void testQString()
    {
        testTypeStreaming<QString>();
    }

    void testQByteArray_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::newRow("null array") << QByteArray();
        QTest::newRow("empty array") << QByteArray();
        QTest::newRow("short acii data") << QByteArray("Hello World");
        QTest::newRow("binary data") << QByteArray("\x02\xA0\x05\x01\xA7\x1B");
        QTest::newRow("null character") << QByteArray("\0", 1);
    }

    void testQDateTime_data()
    {
        QTest::addColumn<QDateTime>("input");
        QTest::newRow("invalid") << QDateTime();
        QTest::newRow("epoch") << QDateTime(QDate(1970, 1, 1), QTime(0, 0, 0), QTimeZone::utc());
        QTest::newRow("UTC date") << QDateTime(QDate(2024, 6, 14), QTime(19, 34, 43), QTimeZone::utc());
        QTest::newRow("local date") << QDateTime(QDate(2024, 6, 14), QTime(19, 34, 43), QTimeZone::LocalTime);
        QTest::newRow("UTC offset") << QDateTime(QDate(2024, 6, 14), QTime(19, 34, 43), QTimeZone::fromSecondsAheadOfUtc(7200));
        QTest::newRow("Timezone") << QDateTime(QDate(2024, 6, 14), QTime(19, 34, 43), QTimeZone("America/New_York"));
    }

    void testQDateTime()
    {
        testTypeStreaming<QDateTime>();
    }

    void testQList_data()
    {
        QTest::addColumn<QList<qint32>>("input");
        QTest::newRow("empty") << QList<qint32>();
        QTest::newRow("single") << QList<qint32>{42};
        QTest::newRow("multiple") << QList<qint32>{42, 1337, -42};
    }

    void testQList()
    {
        testTypeStreaming<QList<qint32>>();
    }

    void testQMap_data()
    {
        QTest::addColumn<QMap<QString, qint32>>("input");
        QTest::newRow("empty") << QMap<QString, qint32>();
        QTest::newRow("single") << QMap<QString, qint32>{{QStringLiteral("foo"), 42}};
        QTest::newRow("multiple") << QMap<QString, qint32>{{QStringLiteral("foo"), 42}, {QStringLiteral("bar"), 1337}, {QStringLiteral("baz"), -42}};
    }

    void testQMap()
    {
        testTypeStreaming<QMap<QString, qint32>>();
    }

    void testQHash_data()
    {
        QTest::addColumn<QHash<QString, qint32>>("input");
        QTest::newRow("empty") << QHash<QString, qint32>();
        QTest::newRow("single") << QHash<QString, qint32>{{QStringLiteral("foo"), 42}};
        QTest::newRow("multiple") << QHash<QString, qint32>{{QStringLiteral("foo"), 42}, {QStringLiteral("bar"), 1337}, {QStringLiteral("baz"), -42}};
    }

    void testQHash()
    {
        testTypeStreaming<QHash<QString, qint32>>();
    }
};

QTEST_GUILESS_MAIN(DataStreamTest)

#include "datastreamtest.moc"
