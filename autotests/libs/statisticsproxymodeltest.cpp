/*
    Copyright (c) 2016 David Faure <faure@kde.org>

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

#include <collectionstatistics.h>
#include <collection.h>
#include <entitytreemodel.h>
#include <QTest>

#include <statisticsproxymodel.h>
#include <QStandardItemModel>
#include "test_model_helpers.h"

using namespace TestModelHelpers;

using Akonadi::StatisticsProxyModel;

#ifndef Q_OS_WIN
void initLocale()
{
    setenv("LC_ALL", "en_US.utf-8", 1);
}
Q_CONSTRUCTOR_FUNCTION(initLocale)
#endif

class StatisticsProxyModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void init();
    void shouldDoNothingIfNoExtraColumns();
    void shouldShowExtraColumns();
    void shouldShowToolTip();
    void shouldHandleDataChanged();
    void shouldHandleDataChangedInExtraColumn();

private:
    QStandardItemModel m_model;
};

// Helper functions

static QString indexToText(const QModelIndex &index)
{
    if (!index.isValid()) {
        return QStringLiteral("invalid");
    }
    return QString::number(index.row()) + QLatin1Char(',') + QString::number(index.column()) + QLatin1Char(',')
           + QString::number(reinterpret_cast<qulonglong>(index.internalPointer()), 16)
           + QLatin1String(" in ") + QString::number(reinterpret_cast<qulonglong>(index.model()), 16);
}

static QString indexRowCol(const QModelIndex &index)
{
    if (!index.isValid()) {
        return QStringLiteral("invalid");
    }
    return QString::number(index.row()) + QLatin1Char(',') + QString::number(index.column());
}

static Akonadi::CollectionStatistics makeStats(qint64 unread, qint64 count, qint64 size)
{
    Akonadi::CollectionStatistics stats;
    stats.setUnreadCount(unread);
    stats.setCount(count);
    stats.setSize(size);
    return stats;
}

void StatisticsProxyModelTest::initTestCase()
{
}

void StatisticsProxyModelTest::init()
{
    // Prepare the source model to use later on
    m_model.clear();
    m_model.appendRow(makeStandardItems(QStringList() << QStringLiteral("A") << QStringLiteral("B") << QStringLiteral("C") << QStringLiteral("D")));
    m_model.item(0, 0)->appendRow(makeStandardItems(QStringList() << QStringLiteral("m") << QStringLiteral("n") << QStringLiteral("o") << QStringLiteral("p")));
    m_model.item(0, 0)->appendRow(makeStandardItems(QStringList() << QStringLiteral("q") << QStringLiteral("r") << QStringLiteral("s") << QStringLiteral("t")));
    m_model.appendRow(makeStandardItems(QStringList() << QStringLiteral("E") << QStringLiteral("F") << QStringLiteral("G") << QStringLiteral("H")));
    m_model.item(1, 0)->appendRow(makeStandardItems(QStringList() << QStringLiteral("x") << QStringLiteral("y") << QStringLiteral("z") << QStringLiteral(".")));
    m_model.setHorizontalHeaderLabels(QStringList() << QStringLiteral("H1") << QStringLiteral("H2") << QStringLiteral("H3") << QStringLiteral("H4"));

    // Set Collection c1 for row A
    Akonadi::Collection c1(1);
    c1.setName(QStringLiteral("c1"));
    c1.setStatistics(makeStats(2, 6, 9)); // unread, count, size in bytes
    m_model.item(0, 0)->setData(QVariant::fromValue(c1), Akonadi::EntityTreeModel::CollectionRole);

    // Set Collection c2 for first child (row m)
    Akonadi::Collection c2(2);
    c2.setName(QStringLiteral("c2"));
    c2.setStatistics(makeStats(1, 3, 4)); // unread, count, size in bytes
    m_model.item(0, 0)->child(0)->setData(QVariant::fromValue(c2), Akonadi::EntityTreeModel::CollectionRole);

    // Set Collection c2 for first child (row m)
    Akonadi::Collection c3(3);
    c3.setName(QStringLiteral("c3"));
    c3.setStatistics(makeStats(0, 1, 1)); // unread, count, size in bytes
    m_model.item(0, 0)->child(1)->setData(QVariant::fromValue(c3), Akonadi::EntityTreeModel::CollectionRole);

    QCOMPARE(extractRowTexts(&m_model, 0), QStringLiteral("ABCD"));
    QCOMPARE(extractRowTexts(&m_model, 0, m_model.index(0, 0)), QStringLiteral("mnop"));
    QCOMPARE(extractRowTexts(&m_model, 1, m_model.index(0, 0)), QStringLiteral("qrst"));
    QCOMPARE(extractRowTexts(&m_model, 1), QStringLiteral("EFGH"));
    QCOMPARE(extractRowTexts(&m_model, 0, m_model.index(1, 0)), QStringLiteral("xyz."));
    QCOMPARE(extractHorizontalHeaderTexts(&m_model), QStringLiteral("H1H2H3H4"));
}

void StatisticsProxyModelTest::shouldDoNothingIfNoExtraColumns()
{
    // Given a statistics proxy with no extra columns
    StatisticsProxyModel pm;
    pm.setExtraColumnsEnabled(false);

    // When setting it to a source model
    pm.setSourceModel(&m_model);

    // Then the proxy should show the same as the model
    QCOMPARE(pm.rowCount(), m_model.rowCount());
    QCOMPARE(pm.columnCount(), m_model.columnCount());

    QCOMPARE(pm.rowCount(pm.index(0, 0)), 2);
    QCOMPARE(pm.index(0, 0).parent(), QModelIndex());

    // (verify that the mapFromSource(mapToSource(x)) == x roundtrip works)
    for (int row = 0; row < pm.rowCount(); ++row) {
        for (int col = 0; col < pm.columnCount(); ++col) {
            QCOMPARE(pm.mapFromSource(pm.mapToSource(pm.index(row, col))), pm.index(row, col));
        }
    }

    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("ABCD"));
    QCOMPARE(extractRowTexts(&pm, 0, pm.index(0, 0)), QStringLiteral("mnop"));
    QCOMPARE(extractRowTexts(&pm, 1, pm.index(0, 0)), QStringLiteral("qrst"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("EFGH"));
    QCOMPARE(extractRowTexts(&pm, 0, pm.index(1, 0)), QStringLiteral("xyz."));
    QCOMPARE(extractHorizontalHeaderTexts(&pm), QStringLiteral("H1H2H3H4"));
}

void StatisticsProxyModelTest::shouldShowExtraColumns()
{
    // Given a extra-columns proxy with three extra columns
    StatisticsProxyModel pm;

    // When setting it to a source model
    pm.setSourceModel(&m_model);

    // Then the proxy should show the extra column
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("ABCD269 B"));
    QCOMPARE(extractRowTexts(&pm, 0, pm.index(0, 0)), QStringLiteral("mnop134 B"));
    QCOMPARE(extractRowTexts(&pm, 1, pm.index(0, 0)), QStringLiteral("qrst 11 B"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("EFGH   "));
    QCOMPARE(extractRowTexts(&pm, 0, pm.index(1, 0)), QStringLiteral("xyz.   "));
    QCOMPARE(extractHorizontalHeaderTexts(&pm), QStringLiteral("H1H2H3H4UnreadTotalSize"));

    // Verify tree structure of proxy
    const QModelIndex secondParent = pm.index(1, 0);
    QVERIFY(!secondParent.parent().isValid());
    QCOMPARE(indexToText(pm.index(0, 0, secondParent).parent()), indexToText(secondParent));
    QCOMPARE(indexToText(pm.index(0, 3, secondParent).parent()), indexToText(secondParent));
    QVERIFY(indexToText(pm.index(0, 4)).startsWith(QLatin1String("0,4,")));
    QCOMPARE(indexToText(pm.index(0, 4, secondParent).parent()), indexToText(secondParent));
    QVERIFY(indexToText(pm.index(0, 5)).startsWith(QLatin1String("0,5,")));
    QCOMPARE(indexToText(pm.index(0, 5, secondParent).parent()), indexToText(secondParent));

    QCOMPARE(pm.index(0, 0).sibling(0, 4).column(), 4);
    QCOMPARE(pm.index(0, 4).sibling(0, 1).column(), 1);

    QVERIFY(!pm.canFetchMore(QModelIndex()));
}

void StatisticsProxyModelTest::shouldShowToolTip()
{
    // Given a extra-columns proxy with three extra columns
    StatisticsProxyModel pm;
    pm.setSourceModel(&m_model);

    // When enabling tooltips and getting the tooltip for the first folder
    pm.setToolTipEnabled(true);
    QString toolTip = pm.index(0, 0).data(Qt::ToolTipRole).toString();

    // Then the tooltip should contain the expected information
    toolTip.remove(QStringLiteral("<strong>"));
    toolTip.remove(QStringLiteral("</strong>"));
    QVERIFY2(toolTip.contains(QLatin1String("Total Messages: 6")), qPrintable(toolTip));
    QVERIFY2(toolTip.contains(QLatin1String("Unread Messages: 2")), qPrintable(toolTip));
    QVERIFY2(toolTip.contains(QLatin1String("Storage Size: 9 B")), qPrintable(toolTip));
    QVERIFY2(toolTip.contains(QLatin1String("Subfolder Storage Size: 5 B")), qPrintable(toolTip));
}

void StatisticsProxyModelTest::shouldHandleDataChanged()
{
    // Given a extra-columns proxy with three extra columns
    StatisticsProxyModel pm;
    pm.setSourceModel(&m_model);
    QSignalSpy dataChangedSpy(&pm, &QAbstractItemModel::dataChanged);

    // When ETM says the collection changed
    m_model.item(0, 0)->setData(QStringLiteral("a"), Qt::EditRole);

    // Then the change should be notified to the proxy -- including the extra columns
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(indexRowCol(dataChangedSpy.at(0).at(0).toModelIndex()), QStringLiteral("0,0"));
    QCOMPARE(indexRowCol(dataChangedSpy.at(0).at(1).toModelIndex()), QStringLiteral("0,6"));
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("aBCD269 B"));
}

void StatisticsProxyModelTest::shouldHandleDataChangedInExtraColumn()
{
    // Given a extra-columns proxy with three extra columns
    StatisticsProxyModel pm;
    pm.setSourceModel(&m_model);
    QSignalSpy dataChangedSpy(&pm, &QAbstractItemModel::dataChanged);

    // When the proxy wants to signal a change in an extra column
    Akonadi::Collection c1(1);
    c1.setName(QStringLiteral("c1"));
    c1.setStatistics(makeStats(3, 5, 8)); // changed: unread, count, size in bytes
    m_model.item(0, 0)->setData(QVariant::fromValue(c1), Akonadi::EntityTreeModel::CollectionRole);

    // Then the change should be available and notified
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("ABCD358 B"));
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(indexRowCol(dataChangedSpy.at(0).at(0).toModelIndex()), QStringLiteral("0,0"));
    QCOMPARE(indexRowCol(dataChangedSpy.at(0).at(1).toModelIndex()), QStringLiteral("0,6"));
}

#include "statisticsproxymodeltest.moc"

QTEST_MAIN(StatisticsProxyModelTest)
