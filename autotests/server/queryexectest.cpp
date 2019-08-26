/*
    Copyright (c) 2019 Daniel Vrátil <dvratil@kde.org>

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
#include <QMap>
#include <QString>

#include "storage/qb/selectquery.h"
#include "storage/qb/insertquery.h"
#include "storage/qb/updatequery.h"
#include "storage/qb/deletequery.h"
#include "storage/qb/aggregatefuncs.h"

#include "fakeakonadiserver.h"
#include "fakedatastore.h"
#include "dbinitializer.h"
#include "shared/aktest.h"
#include "entities.h"

using namespace Akonadi::Server;

class QueryExecTest : public QObject
{
    Q_OBJECT
public:
    explicit QueryExecTest()
        : QObject()
    {
        FakeAkonadiServer::instance()->init();
    }

    ~QueryExecTest()
    {
        FakeAkonadiServer::instance()->quit();
    }

private Q_SLOTS:
    void testSelect()
    {
        auto &db = *DataStore::self();
        auto query = Qb::Select(db)
                .from(Collection::tableName())
                .columns(Collection::nameColumn(), Collection::parentIdColumn())
                .where({Collection::parentIdColumn(), Qb::Compare::Is, QVariant{}});
        QVERIFY(query.exec());
        QVERIFY(!query.error().isValid());
        int count = 0;
        for (const auto &result: query) {
            QVERIFY(result.at<QString>(1).isNull());
            count++;
        }
        QCOMPARE(count, 3);
    }

    void testInsert()
    {
        auto &db = *DataStore::self();
        auto insert = Qb::Insert(db)
                .into(MimeType::tableName())
                .value(MimeType::nameColumn(), QStringLiteral("foo/bar"));
        QVERIFY(insert.exec());
        QVERIFY(!insert.error().isValid());
        const auto insertId = insert.insertId();
        QVERIFY(insertId.has_value());

        auto select = Qb::Select(db)
                .from(MimeType::tableName())
                .column(MimeType::nameColumn())
                .where({MimeType::idColumn(), Qb::Compare::Equals, *insertId});
        QVERIFY(select.exec());
        QVERIFY(!select.error().isValid());
        QCOMPARE(select.begin()->at<QString>(0), QStringLiteral("foo/bar"));
    }

    void testUpdate()
    {
        auto &db = *DataStore::self();

        auto insert = Qb::Insert(db)
                .into(MimeType::tableName())
                .value(MimeType::nameColumn(), QStringLiteral("bar/foo"));
        QVERIFY(insert.exec());
        QVERIFY(!insert.error().isValid());
        const auto insertId = insert.insertId();
        QVERIFY(insertId.has_value());

        auto update = Qb::Update(db)
                .table(MimeType::tableName())
                .value(MimeType::nameColumn(), QStringLiteral("baz/blah"))
                .where({MimeType::idColumn(), Qb::Compare::Equals, *insertId});
        QVERIFY(update.exec());
        QVERIFY(!update.error().isValid());

        auto select = Qb::Select(db)
                .from(MimeType::tableName())
                .column(MimeType::nameColumn())
                .where({MimeType::idColumn(), Qb::Compare::Equals, *insertId});
        QVERIFY(select.exec());
        QVERIFY(!select.error().isValid());
        QCOMPARE(select.begin()->at<QString>(0), QStringLiteral("baz/blah"));
    }

    void testDelete()
    {
        auto &db = *DataStore::self();

        auto insert = Qb::Insert(db)
                .into(MimeType::tableName())
                .value(MimeType::nameColumn(), QStringLiteral("bla/bla"));
        QVERIFY(insert.exec());
        QVERIFY(!insert.error().isValid());
        const auto insertId = insert.insertId();
        QVERIFY(insertId.has_value());

        auto select = Qb::Select(db)
                .from(MimeType::tableName())
                .column(Qb::Count())
                .where({MimeType::idColumn(), Qb::Compare::Equals, *insertId});
        QVERIFY(select.exec());
        QCOMPARE(select.begin()->at<int>(0), 1);

        auto del = Qb::Delete(db)
                .from(MimeType::tableName())
                .where({MimeType::idColumn(), Qb::Compare::Equals, *insertId});
        QVERIFY(del.exec());
        QVERIFY(!del.error().isValid());

        QVERIFY(select.exec());
        QCOMPARE(select.begin()->at<int>(0), 0);
    }
};

AKTEST_FAKESERVER_MAIN(QueryExecTest)

#include "queryexectest.moc"


