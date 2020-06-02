/*
    Copyright (c) 2018  Daniel Vrátil <dvratil@kde.org>

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
#include <QTest>

#include <shared/akranges.h>

#include <iostream>

using namespace AkRanges;

namespace {

int transformFreeFunc(int i)
{
    return i * 2;
}

struct TransformHelper
{
public:
    static int transform(int i)
    {
        return transformFreeFunc(i);
    }

    int operator()(int i) const
    {
        return transformFreeFunc(i);
    }
};

bool filterFreeFunc(int i)
{
    return i % 2 == 0;
}

struct FilterHelper
{
public:
    static bool filter(int i)
    {
        return filterFreeFunc(i);
    }

    bool operator()(int i)
    {
        return filterFreeFunc(i);
    }
};

} // namespace

class AkRangesTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        qSetGlobalQHashSeed(0);
    }

    void testTraits()
    {
        QVERIFY(AkTraits::isAppendable<QVector<int>>);
        QVERIFY(!AkTraits::isInsertable<QVector<int>>);
        QVERIFY(AkTraits::isReservable<QVector<int>>);

        QVERIFY(!AkTraits::isAppendable<QSet<int>>);
        QVERIFY(AkTraits::isInsertable<QSet<int>>);
        QVERIFY(AkTraits::isReservable<QSet<int>>);

        QVERIFY(!AkTraits::isAppendable<QString>);
        QVERIFY(!AkTraits::isInsertable<QString>);
        QVERIFY(AkTraits::isReservable<QString>);
    }

    void testContainerConversion()
    {
        {
            QVector<int> in = { 1, 2, 3, 4, 5 };
            QCOMPARE(in | Actions::toQList, in.toList());
            QCOMPARE(in | Actions::toQList | Actions::toQVector, in);
            QCOMPARE(in | Actions::toQSet, in.toList().toSet()); // clazy:exclude=container-anti-pattern
        }
        {
            QList<int> in = { 1, 2, 3, 4, 5 };
            QCOMPARE(in | Actions::toQVector, in.toVector());
            QCOMPARE(in | Actions::toQVector | Actions::toQList, in);
            QCOMPARE(in | Actions::toQSet, in.toSet());
        }
    }

    void testAssociativeContainerConversion()
    {
        QVector<std::pair<int, QString>> in = {{1, QStringLiteral("One")}, {2, QStringLiteral("Two")}, {3, QStringLiteral("Three")}};
        QMap<int, QString> out = {{1, QStringLiteral("One")}, {2, QStringLiteral("Two")}, {3, QStringLiteral("Three")}};
        QCOMPARE(in | Actions::toQMap, out);
    }

    void testRangeConversion()
    {
        {
            QList<int> in = { 1, 2, 3, 4, 5 };
            AkRanges::detail::Range<QList<int>::const_iterator> range(in.cbegin(), in.cend());
            QCOMPARE(range | Actions::toQVector, QVector<int>::fromList(in));
        }

        {
            QVector<int> in = { 1, 2, 3, 4, 5 };
            AkRanges::detail::Range<QVector<int>::const_iterator> range(in.cbegin(), in.cend());
            QCOMPARE(range | Actions::toQList, in.toList());
        }

        {
            QVector<std::pair<int, QString>> in = {{1, QStringLiteral("One")}, {2, QStringLiteral("Two")}, {3, QStringLiteral("Three")}};
            QMap<int, QString> out = {{1, QStringLiteral("One")}, {2, QStringLiteral("Two")}, {3, QStringLiteral("Three")}};
            AkRanges::detail::Range<QVector<std::pair<int, QString>>::const_iterator> range(in.cbegin(), in.cend());
            QCOMPARE(range | Actions::toQMap, out);
        }
    }

    void testTransform()
    {
        QList<int> in = { 1, 2, 3, 4, 5 };
        QList<int> out = { 2, 4, 6, 8, 10 };
        QCOMPARE(in | Views::transform([](int i) { return i * 2; }) | Actions::toQList, out);
        QCOMPARE(in | Views::transform(transformFreeFunc) | Actions::toQList, out);
        QCOMPARE(in | Views::transform(&TransformHelper::transform) | Actions::toQList, out);
        QCOMPARE(in | Views::transform(TransformHelper()) | Actions::toQList, out);
    }

private:
    class CopyCounter {
    public:
        CopyCounter() = default;
        CopyCounter(const CopyCounter &other)
            : copyCount(other.copyCount + 1), transformed(other.transformed)
        {}
        CopyCounter(CopyCounter &&other) = default;
        CopyCounter &operator=(const CopyCounter &other) {
            copyCount = other.copyCount + 1;
            transformed = other.transformed;
            return *this;
        }
        CopyCounter &operator=(CopyCounter &&other) = default;
        ~CopyCounter() = default;

        int copyCount = 0;
        bool transformed = false;
    };

private Q_SLOTS:

    void testTransformCopyCount()
    {
        {
            QList<CopyCounter> in = { {} }; // 1st copy (QList::append())
            QList<CopyCounter> out = in
                | Views::transform([](const auto &c) {
                        CopyCounter r(c);   // 2nd copy (expected)
                        r.transformed = true;
                        return r; })
                | Actions::toQList;         // 3rd copy (QList::append())
            QCOMPARE(out.size(), in.size());
            QCOMPARE(out[0].copyCount, 3);
            QCOMPARE(out[0].transformed, true);
        }

        {
            QVector<CopyCounter> in(1); // construct vector of one element, so no copying
                                        // occurs at initialization
            QVector<CopyCounter> out = in
                | Views::transform([](const auto &c) {
                        CopyCounter r(c);   // 1st copy
                        r.transformed = true;
                        return r; })
                | Actions::toQVector;
            QCOMPARE(out.size(), in.size());
            QCOMPARE(out[0].copyCount, 1);
            QCOMPARE(out[0].transformed, true);
        }
    }

    void testTransformConvert()
    {
        {
            QList<int> in = { 1, 2, 3, 4, 5 };
            QVector<int> out = { 2, 4, 6, 8, 10 };
            QCOMPARE(in | Views::transform([](int i) { return i * 2; }) | Actions::toQVector, out);
        }

        {
            QVector<int> in = { 1, 2, 3, 4, 5 };
            QList<int> out = { 2, 4, 6, 8, 10 };
            QCOMPARE(in | Views::transform([](int i) { return i * 2; }) | Actions::toQList, out);
        }
    }

    void testCreateRange()
    {
        {
            QList<int> in = { 1, 2, 3, 4, 5, 6 };
            QList<int> out = { 3, 4, 5 };
            QCOMPARE(Views::range(in.begin() + 2, in.begin() + 5) | Actions::toQList, out);
        }
    }

    void testRangeWithTransform()
    {
        {
            QList<int> in = { 1, 2, 3, 4, 5, 6 };
            QList<int> out = { 6, 8, 10 };
            QCOMPARE(Views::range(in.begin() + 2, in.begin() + 5)
                        | Views::transform([](int i) { return i  * 2; })
                        | Actions::toQList,
                     out);
        }
    }

    void testTransformType()
    {
        {
            QStringList in = { QStringLiteral("foo"), QStringLiteral("foobar"), QStringLiteral("foob") };
            QList<int> out = { 3, 6, 4 };
            QCOMPARE(in | Views::transform([](const auto &str) { return str.size(); }) | Actions::toQList, out);
        }
    }

    void testFilter()
    {
        {
            QList<int> in = { 1, 2, 3, 4, 5, 6, 7, 8 };
            QList<int> out = { 2, 4, 6, 8 };
            QCOMPARE(in | Views::filter([](int i) { return i % 2 == 0; })
                        | Actions::toQList,
                     out);
            QCOMPARE(in | Views::filter(filterFreeFunc) | Actions::toQList, out);
            QCOMPARE(in | Views::filter(&FilterHelper::filter) | Actions::toQList, out);
            QCOMPARE(in | Views::filter(FilterHelper()) | Actions::toQList, out);
        }
    }

    void testFilterTransform()
    {
        {
            QStringList in = { QStringLiteral("foo"), QStringLiteral("foobar"), QStringLiteral("foob") };
            QList<int> out = { 6 };
            QCOMPARE(in | Views::transform(&QString::size)
                        | Views::filter([](int i) { return i > 5; })
                        | Actions::toQList,
                     out);
            QCOMPARE(in | Views::filter([](const auto &str) { return str.size() > 5; })
                        | Views::transform(&QString::size)
                        | Actions::toQList,
                     out);
        }
    }

    void testTemporaryContainer()
    {
        const auto func = []{
            QStringList rv;
            for (int i = 0; i < 5; i++) {
                rv.push_back(QString::number(i));
            }
            return rv;
        };
        {
            QList<int> out = { 0, 2, 4 };
            QCOMPARE(func() | Views::transform([](const auto &str) { return str.toInt(); })
                            | Views::filter([](int i) { return i % 2 == 0; })
                            | Actions::toQList,
                     out);
        }
        {
            QList<int> out = { 0, 2, 4 };
            QCOMPARE(func() | Views::filter([](const auto &v) { return v.toInt() % 2 == 0; })
                            | Views::transform([](const auto &str) { return str.toInt(); })
                            | Actions::toQList,
                     out);
        }
    }

    void testTemporaryRange()
    {
        const auto func = []{
            QStringList rv;
            for (int i = 0; i < 5; ++i) {
                rv.push_back(QString::number(i));
            }
            return rv | Views::transform([](const auto &str) { return str.toInt(); });
        };
        QList<int> out = { 1, 3 };
        QCOMPARE(func() | Views::filter([](int i) { return i % 2 == 1; })
                        | Actions::toQList,
                 out);
    }

private:
    struct ForEachCallable
    {
    public:
        explicit ForEachCallable(QList<int> &out)
            : mOut(out) {}

        void operator()(int i) {
            mOut.push_back(i);
        }

        static void append(int i) {
            sOut.push_back(i);
        }
        static void clear() {
            sOut.clear();
        }
        static QList<int> sOut;
    private:
        QList<int> &mOut;
    };

private Q_SLOTS:
    void testForEach()
    {
        const QList<int> in = { 1, 2, 3, 4, 5, 6 };
        {
            QList<int> out;
            in | Actions::forEach([&out](int v) { out.push_back(v); });
            QCOMPARE(out, in);
        }
        {
            QList<int> out;
            in | Actions::forEach(ForEachCallable(out));
            QCOMPARE(out, in);
        }
        {
            ForEachCallable::clear();
            in | Actions::forEach(&ForEachCallable::append);
            QCOMPARE(ForEachCallable::sOut, in);
        }
        {
            QList<int> out;
            QCOMPARE(in | Actions::forEach([&out](int v) { out.push_back(v); })
                        | Views::filter([](int v){ return v % 2 == 0; })
                        | Views::transform([](int v) { return v * 2; })
                        | Actions::toQList,
                     QList<int>({ 4, 8, 12 }));
            QCOMPARE(out, in);
        }

    }

private:
    template<template<typename, typename> class Container>
    void testKeysValuesHelper()
    {
        const Container<int, QString> in = {
            { 1, QStringLiteral("1") },
            { 2, QStringLiteral("2") },
            { 3, QStringLiteral("3") }
        };

        {
            const QList<int> out = { 1, 2, 3 };
            QCOMPARE(out, in | Views::keys | Actions::toQList);
        }
        {
            const QStringList out = { QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3") };
            QCOMPARE(out, in | Views::values | Actions::toQList);
        }
    }

private Q_SLOTS:
    void testKeysValues()
    {
        testKeysValuesHelper<QMap>();
        testKeysValuesHelper<QHash>();
    }

    void testAll()
    {
        const QList<int> vals = { 2, 4, 6, 8, 10 };
        QVERIFY(vals | Actions::all([](int v) { return v % 2 == 0; }));
        QVERIFY(!(vals | Actions::all([](int v) { return v % 2 == 1; })));
    }

    void testAny()
    {
        const QList<int> vals = { 1, 3, 5, 7, 9 };
        QVERIFY(vals | Actions::any([](int v) { return v % 2 == 1; }));
        QVERIFY(!(vals | Actions::any([](int v) { return v % 2 == 0; })));
    }

    void testNone()
    {
        const QList<int> vals = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        QVERIFY(vals | Views::filter([](int i) { return i % 2 == 0; }) | Actions::none([](int i) { return i % 2 == 1; }));
        QVERIFY(!(vals | Views::filter([](int i) { return i % 2 == 0; }) | Actions::none([](int i) { return i % 2 == 0; })));
    }
};

QList<int> AkRangesTest::ForEachCallable::sOut;

QTEST_GUILESS_MAIN(AkRangesTest)

#include "akrangestest.moc"
