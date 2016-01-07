/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#include <qtest_akonadi.h>

#include "searchquery.h"
#include <qjson/serializer.h>
#include <qjson/parser.h>

using namespace Akonadi;

class SearchQueryTest : public QObject
{
    Q_OBJECT
private:
    void verifySimpleTerm(const QVariantMap &json, const SearchTerm &term, bool *ok)
    {
        *ok = false;
        QCOMPARE(term.subTerms().count(), 0);
        QVERIFY(json.contains(QStringLiteral("key")));
        QCOMPARE(json[QStringLiteral("key")].toString(), term.key());
        QVERIFY(json.contains(QStringLiteral("value")));
        QCOMPARE(json[QStringLiteral("value")], term.value());
        QVERIFY(json.contains(QStringLiteral("cond")));
        QCOMPARE(static_cast<SearchTerm::Condition>(json[QStringLiteral("cond")].toInt()), term.condition());
        QVERIFY(json.contains(QStringLiteral("negated")));
        QCOMPARE(json[QStringLiteral("negated")].toBool(), term.isNegated());
        *ok = true;
    }

private Q_SLOTS:
    void testSerializer()
    {
        QJson::Parser parser;
        bool ok = false;

        {
            SearchQuery query;
            query.addTerm(QStringLiteral("body"), QStringLiteral("test string"), SearchTerm::CondContains);

            ok = false;
            QVariantMap map = parser.parse(query.toJSON(), &ok).toMap();
            QVERIFY(ok);

            QCOMPARE(static_cast<SearchTerm::Relation>(map[QStringLiteral("rel")].toInt()), SearchTerm::RelAnd);
            const QVariantList subTerms = map[QStringLiteral("subTerms")].toList();
            QCOMPARE(subTerms.size(), 1);

            ok = false;
            verifySimpleTerm(subTerms.first().toMap(), query.term().subTerms()[0], &ok);
            QVERIFY(ok);
        }

        {
            SearchQuery query(SearchTerm::RelOr);
            query.addTerm(SearchTerm(QStringLiteral("to"), QStringLiteral("test@test.user"), SearchTerm::CondEqual));
            SearchTerm term2(QStringLiteral("subject"), QStringLiteral("Hello"), SearchTerm::CondContains);
            term2.setIsNegated(true);
            query.addTerm(term2);

            ok = false;
            QVariantMap map = parser.parse(query.toJSON(), &ok).toMap();
            QVERIFY(ok);

            QCOMPARE(static_cast<SearchTerm::Relation>(map[QStringLiteral("rel")].toInt()), query.term().relation());
            const QVariantList subTerms = map[QStringLiteral("subTerms")].toList();
            QCOMPARE(subTerms.size(), query.term().subTerms().count());

            for (int i = 0; i < subTerms.size(); ++i) {
                ok = false;
                verifySimpleTerm(subTerms[i].toMap(), query.term().subTerms()[i], &ok);
                QVERIFY(ok);
            }
        }
    }

    void testParser()
    {
        QJson::Serializer serializer;
        bool ok = false;

        {
            QVariantList subTerms;
            QVariantMap termJSON;
            termJSON[QStringLiteral("key")] = QStringLiteral("created");
            termJSON[QStringLiteral("value")] = QDateTime(QDate(2014, 01, 24), QTime(17, 49, 00));
            termJSON[QStringLiteral("cond")] = static_cast<int>(SearchTerm::CondGreaterOrEqual);
            termJSON[QStringLiteral("negated")] = true;
            subTerms << termJSON;

            termJSON[QStringLiteral("key")] = QStringLiteral("subject");
            termJSON[QStringLiteral("value")] = QStringLiteral("Hello");
            termJSON[QStringLiteral("cond")] = static_cast<int>(SearchTerm::CondEqual);
            termJSON[QStringLiteral("negated")] = false;
            subTerms << termJSON;

            QVariantMap map;
            map[QStringLiteral("rel")] = static_cast<int>(SearchTerm::RelAnd);
            map[QStringLiteral("subTerms")] = subTerms;

#if !defined( USE_QJSON_0_8 )
            const QByteArray json = serializer.serialize(map);
            QVERIFY(!json.isNull());
#else
            ok = false;
            const QByteArray json = serializer.serialize(map, &ok);
            QVERIFY(ok);
#endif

            const SearchQuery query = SearchQuery::fromJSON(json);
            QVERIFY(!query.isNull());
            const SearchTerm term = query.term();

            QCOMPARE(static_cast<SearchTerm::Relation>(map[QStringLiteral("rel")].toInt()), term.relation());
            QCOMPARE(subTerms.count(), term.subTerms().count());

            for (int i = 0; i < subTerms.count(); ++i) {
                ok = false;
                verifySimpleTerm(subTerms.at(i).toMap(), term.subTerms()[i], &ok);
                QVERIFY(ok);
            }
        }
    }

    void testFullQuery()
    {
        {
            SearchQuery query;
            query.addTerm("key", "value");
            const QByteArray serialized = query.toJSON();
            QCOMPARE(SearchQuery::fromJSON(serialized), query);
        }
        {
            SearchQuery query;
            query.setLimit(10);
            query.addTerm("key", "value");
            const QByteArray serialized = query.toJSON();
            QCOMPARE(SearchQuery::fromJSON(serialized), query);
        }
    }

};

QTEST_AKONADIMAIN(SearchQueryTest, NoGUI)

#include "searchquerytest.moc"
