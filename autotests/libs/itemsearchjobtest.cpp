/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qtest_akonadi.h"

#include "agentinstance.h"
#include "agentmanager.h"
#include "collection.h"
#include "item.h"
#include "itemsearchjob.h"
#include "searchquery.h"

Q_DECLARE_METATYPE(QSet<qint64>)
Q_DECLARE_METATYPE(Akonadi::SearchQuery)

using namespace Akonadi;
class ItemSearchJobTest : public QObject
{
    Q_OBJECT
private:
    Akonadi::SearchQuery createQuery(const QString &key, const QSet<qint64> &resultSet)
    {
        Akonadi::SearchQuery query;
        foreach (qint64 id, resultSet) {
            query.addTerm(Akonadi::SearchTerm(key, id));
        }
        return query;
    }

private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
    }

    void testItemSearch_data()
    {
        QTest::addColumn<bool>("remoteSearchEnabled");
        QTest::addColumn<SearchQuery>("query");
        QTest::addColumn<QSet<qint64>>("resultSet");

        {
            QSet<qint64> resultSet;
            resultSet << 1 << 2 << 3;
            QTest::newRow("plugin search") << false << createQuery(QStringLiteral("plugin"), resultSet) << resultSet;
        }
        {
            QSet<qint64> resultSet;
            resultSet << 1 << 2 << 3;
            QTest::newRow("resource search") << true << createQuery(QStringLiteral("resource"), resultSet) << resultSet;
        }
        {
            QSet<qint64> resultSet;
            resultSet << 1 << 2 << 3 << 4;
            Akonadi::SearchQuery query;
            query.addTerm(Akonadi::SearchTerm(QStringLiteral("plugin"), 1));
            query.addTerm(Akonadi::SearchTerm(QStringLiteral("resource"), 2));
            query.addTerm(Akonadi::SearchTerm(QStringLiteral("plugin"), 3));
            query.addTerm(Akonadi::SearchTerm(QStringLiteral("resource"), 4));
            QTest::newRow("mixed search: results are merged") << true << query << resultSet;
        }
    }

    void testItemSearch()
    {
        QFETCH(bool, remoteSearchEnabled);
        QFETCH(SearchQuery, query);
        QFETCH(QSet<qint64>, resultSet);

        ItemSearchJob *itemSearchJob = new ItemSearchJob(query, this);
        itemSearchJob->setRemoteSearchEnabled(remoteSearchEnabled);
        itemSearchJob->setSearchCollections(Collection::List() << Collection::root());
        itemSearchJob->setRecursive(true);
        AKVERIFYEXEC(itemSearchJob);
        QSet<qint64> actualResultSet;
        foreach (const Item &item, itemSearchJob->items()) {
            actualResultSet << item.id();
        }
        qDebug() << actualResultSet << resultSet;
        QCOMPARE(actualResultSet, resultSet);
    }
};

QTEST_AKONADIMAIN(ItemSearchJobTest)

#include "itemsearchjobtest.moc"
