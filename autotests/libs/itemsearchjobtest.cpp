/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "test_utils.h"

#include "collection.h"
#include "item.h"
#include "agentmanager.h"
#include "agentinstance.h"
#include "itemsearchjob.h"
#include "searchquery.h"

Q_DECLARE_METATYPE(QSet<qint64>)
Q_DECLARE_METATYPE(Akonadi::SearchQuery)

using namespace Akonadi;
class ItemSearchJobTest : public QObject
{
    Q_OBJECT
private:
    Akonadi::SearchQuery createQuery(const QString &key, const QSet< qint64 > &resultSet)
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
        AkonadiTest::setAllResourcesOffline();
        Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance(QStringLiteral("akonadi_knut_resource_0"));
        QVERIFY(agent.isValid());
        agent.setIsOnline(true);
        QTRY_VERIFY(agent.isOnline());
    }

    void testItemSearch_data()
    {
        QTest::addColumn<bool>("remoteSearchEnabled");
        QTest::addColumn<SearchQuery>("query");
        QTest::addColumn<QSet<qint64> >("resultSet");

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
//         qDebug() << actualResultSet << resultSet;
        QCOMPARE(actualResultSet, resultSet);
    }

};

QTEST_AKONADIMAIN(ItemSearchJobTest)

#include "itemsearchjobtest.moc"
