/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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

#include <storage/parttypehelper.h>
#include <aktest.h>

#include <QObject>
#include <QTest>

#define QL1S(x) QStringLiteral(x)

using namespace Akonadi::Server;

class PartTypeHelperTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testParseFqName_data()
    {
        QTest::addColumn<QString>("fqName");
        QTest::addColumn<QString>("ns");
        QTest::addColumn<QString>("name");
        QTest::addColumn<bool>("shouldThrow");

        QTest::newRow("empty") << QString() << QString() << QString() << true;
        QTest::newRow("valid") << "PLD:RFC822" << "PLD" << "RFC822" << false;
        QTest::newRow("no separator") << "ABC" << QString() << QString() << true;
        QTest::newRow("no ns") << ":RFC822" << QString() << QString() << true;
        QTest::newRow("no name") << "PLD:" << QString() << QString() << true;
        QTest::newRow("too many separators") << "A:B:C" << QString() << QString() << true;
    }

    void testParseFqName()
    {
        QFETCH(QString, fqName);
        QFETCH(QString, ns);
        QFETCH(QString, name);
        QFETCH(bool, shouldThrow);

        std::pair<QString, QString> p;
        bool didThrow = false;
        try {
            p = PartTypeHelper::parseFqName(fqName);
        } catch (const PartTypeException &e) {
            didThrow = true;
        }

        QCOMPARE(didThrow, shouldThrow);
        QCOMPARE(p.first, ns);
        QCOMPARE(p.second, name);
    }

    void testConditionFromName()
    {
        Query::Condition c = PartTypeHelper::conditionFromFqName(QL1S("PLD:RFC822"));
        QVERIFY(!c.isEmpty());
        QCOMPARE(c.subConditions().size(), 2);
    }

    void testConditionFromNames()
    {
        Query::Condition c = PartTypeHelper::conditionFromFqNames(QStringList() << QL1S("PLD:RFC822") << QL1S("PLD:HEAD") << QL1S("PLD:ENVELOPE"));
        QVERIFY(!c.isEmpty());
        QCOMPARE(c.subConditions().size(), 3);
        Q_FOREACH (const Query::Condition &subC, c.subConditions()) {
            QCOMPARE(subC.subConditions().size(), 2);
        }
    }
};

AKTEST_MAIN(PartTypeHelperTest)

#include "parttypehelpertest.moc"
