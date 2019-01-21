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

using namespace Akonadi;

class AkRangesTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testContainerConversion()
    {
        {
            QVector<int> in = { 1, 2, 3, 4, 5 };
            QCOMPARE(in | Akonadi::toQList, in.toList());
            QCOMPARE(in | Akonadi::toQList | Akonadi::toQVector, in);
            QCOMPARE(in | Akonadi::toQSet, in.toList().toSet());
        }
        {
            QList<int> in = { 1, 2, 3, 4, 5 };
            QCOMPARE(in | Akonadi::toQVector, in.toVector());
            QCOMPARE(in | Akonadi::toQVector | toQList, in);
            QCOMPARE(in | Akonadi::toQSet, in.toSet());
        }
    }

    void testRangeConversion()
    {
        {
            QList<int> in = { 1, 2, 3, 4, 5 };
            Akonadi::detail::Range<QList<int>::const_iterator> range(in.cbegin(), in.cend());
            QCOMPARE(in | Akonadi::toQVector, QVector<int>::fromList(in));
        }

        {
            QVector<int> in = { 1, 2, 3, 4, 5 };
            Akonadi::detail::Range<QVector<int>::const_iterator> range(in.cbegin(), in.cend());
            QCOMPARE(in | toQList, in.toList());
        }
    }

    void testTransform()
    {
        QList<int> in = { 1, 2, 3, 4, 5 };
        QList<int> out = { 2, 4, 6, 8, 10 };
        QCOMPARE(in | transform([](int i) { return i * 2; }) | toQList, out);
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

        int copyCount = 0;
        bool transformed = false;
    };

private Q_SLOTS:

    void testTransformCopyCount()
    {
        {
            QList<CopyCounter> in = { {} }; // 1st copy (QList::append())
            QList<CopyCounter> out = in 
                | transform([](const auto &c) {
                        CopyCounter r(c);   // 2nd copy (expected)
                        r.transformed = true;
                        return r; })
                | toQList;                  // 3rd copy (QList::append())
            QCOMPARE(out.size(), in.size());
            QCOMPARE(out[0].copyCount, 3);
            QCOMPARE(out[0].transformed, true);
        }

        {
            QVector<CopyCounter> in(1); // construct vector of one element, so no copying
                                        // occurs at initialization
            QVector<CopyCounter> out = in
                | transform([](const auto &c) {
                        CopyCounter r(c);   // 1st copy
                        r.transformed = true;
                        return r; })
                | toQVector;
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
            QCOMPARE(in | transform([](int i) { return i * 2; }) | toQVector, out);
        }

        {
            QVector<int> in = { 1, 2, 3, 4, 5 };
            QList<int> out = { 2, 4, 6, 8, 10 };
            QCOMPARE(in | transform([](int i) { return i * 2; }) | toQList, out);
        }
    }

    void testCreateRange()
    {
        {
            QList<int> in = { 1, 2, 3, 4, 5, 6 };
            QList<int> out = { 3, 4, 5 };
            QCOMPARE(range(in.begin() + 2, in.begin() + 5) | toQList, out);
        }
    }

    void testRangeWithTransform()
    {
        {
            QList<int> in = { 1, 2, 3, 4, 5, 6 };
            QList<int> out = { 6, 8, 10 };
            QCOMPARE(range(in.begin() + 2, in.begin() + 5) | transform([](int i) { return i  * 2; }) | toQList, out);
        }
    }

    void testTransformType()
    {
        {
            QStringList in = { QStringLiteral("foo"), QStringLiteral("foobar"), QStringLiteral("foob") };
            QList<int> out = { 3, 6, 4 };
            QCOMPARE(in | transform([](const auto &str) { return str.size(); }) | toQList, out);
        }
    }

    void testFilter()
    {
        {
            QList<int> in = { 1, 2, 3, 4, 5, 6, 7, 8 };
            QList<int> out = { 2, 4, 6, 8 };
            QCOMPARE(in | filter([](int i) { return i % 2 == 0; })
                        | toQList,
                     out);
        }
    }

    void testFilterTransform()
    {
        {
            QStringList in = { QStringLiteral("foo"), QStringLiteral("foobar"), QStringLiteral("foob") };
            QList<int> out = { 6 };
            QCOMPARE(in | transform([](const auto &str) { return str.size(); })
                        | filter([](int i) { return i > 5; })
                        | toQList,
                     out);
            QCOMPARE(in | filter([](const auto &str) { return str.size() > 5; })
                        | transform([](const auto &str) { return str.size(); })
                        | toQList,
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
            QCOMPARE(func() | transform([](const auto &str) { return str.toInt(); })
                            | filter([](int i) { return i % 2 == 0; })
                            | toQList,
                     out);
        }
        {
            QList<int> out = { 0, 2, 4 };
            QCOMPARE(func() | filter([](const auto &v) { return v.toInt() % 2 == 0; })
                            | transform([](const auto &str) { return str.toInt(); })
                            | toQList,
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
            return rv | transform([](const auto &str) { return str.toInt(); });
        };
        QList<int> out = { 1, 3 };
        QCOMPARE(func() | filter([](int i) { return i % 2 == 1; })
                        | toQList,
                 out);
    }

    void testForEach()
    {
        const QList<int> in = { 1, 2, 3, 4, 5, 6 };
        {
            QList<int> out;
            in | forEach([&out](int v) { out.push_back(v); });
            QCOMPARE(out, in);
        }
        {
            QList<int> out;
            QCOMPARE(in | forEach([&out](int v) { out.push_back(v); })
                        | filter([](int v){ return v % 2 == 0; })
                        | transform([](int v) { return v * 2; })
                        | toQList,
                     QList<int>({ 4, 8, 12 }));
            QCOMPARE(out, in);
        }

    }
};

QTEST_GUILESS_MAIN(AkRangesTest)

#include "akrangestest.moc"
