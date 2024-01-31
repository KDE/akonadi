/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "storage/parttypehelper.h"
#include "aktest.h"

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
        QTest::newRow("valid") << "PLD:RFC822"
                               << "PLD"
                               << "RFC822" << false;
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
        const auto subCs = c.subConditions();
        for (const Query::Condition &subC : subCs) {
            QCOMPARE(subC.subConditions().size(), 2);
        }
    }
};

AKTEST_MAIN(PartTypeHelperTest)

#include "parttypehelpertest.moc"
