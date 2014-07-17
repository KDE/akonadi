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

#include <akonadi/searchquery.h>
#include <qjson/serializer.h>
#include <qjson/parser.h>
#include <boost/concept_check.hpp>

using namespace Akonadi;

class SearchQueryTest : public QObject
{
  Q_OBJECT
  private:
    void verifySimpleTerm( const QVariantMap &json, const SearchTerm &term, bool *ok )
    {
      *ok = false;
      QCOMPARE( term.subTerms().count(), 0 );
      QVERIFY( json.contains( QLatin1String( "key" ) ) );
      QCOMPARE( json[QLatin1String( "key" )].toString(), term.key() );
      QVERIFY( json.contains( QLatin1String( "value" ) ) );
      QCOMPARE( json[QLatin1String( "value" )], term.value() );
      QVERIFY( json.contains( QLatin1String( "cond" ) ) );
      QCOMPARE( static_cast<SearchTerm::Condition>( json[QLatin1String( "cond" )].toInt() ), term.condition() );
      QVERIFY( json.contains( QLatin1String( "negated" ) ) );
      QCOMPARE( json[QLatin1String( "negated" )].toBool(), term.isNegated() );
      *ok = true;
    }

  private Q_SLOTS:
    void testSerializer()
    {
      QJson::Parser parser;
      bool ok = false;

      {
        SearchQuery query;
        query.addTerm( QLatin1String( "body" ), QLatin1String( "test string"), SearchTerm::CondContains );

        ok = false;
        QVariantMap map = parser.parse( query.toJSON(), &ok ).toMap();
        QVERIFY( ok );

        QCOMPARE( static_cast<SearchTerm::Relation>( map[QLatin1String( "rel" )].toInt() ), SearchTerm::RelAnd );
        const QVariantList subTerms = map[QLatin1String( "subTerms" )].toList();
        QCOMPARE( subTerms.size(), 1 );

        ok = false;
        verifySimpleTerm( subTerms.first().toMap(), query.term().subTerms()[0], &ok );
        QVERIFY( ok );
      }

      {
        SearchQuery query( SearchTerm::RelOr );
        query.addTerm( SearchTerm( QLatin1String( "to" ), QLatin1String( "test@test.user" ), SearchTerm::CondEqual ) );
        SearchTerm term2( QLatin1String( "subject"), QLatin1String( "Hello" ), SearchTerm::CondContains );
        term2.setIsNegated( true );
        query.addTerm( term2 );

        ok = false;
        QVariantMap map = parser.parse( query.toJSON(), &ok ).toMap();
        QVERIFY( ok );

        QCOMPARE( static_cast<SearchTerm::Relation>( map[QLatin1String( "rel" )].toInt() ), query.term().relation() );
        const QVariantList subTerms = map[QLatin1String( "subTerms" )].toList();
        QCOMPARE( subTerms.size(), query.term().subTerms().count() );

        for ( int i = 0; i < subTerms.size(); ++i ) {
          ok = false;
          verifySimpleTerm( subTerms[i].toMap(), query.term().subTerms()[i], &ok);
          QVERIFY( ok );
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
        termJSON[QLatin1String( "key" )] = QLatin1String( "created" );
        termJSON[QLatin1String( "value" )] = QDateTime( QDate( 2014, 01, 24 ), QTime( 17, 49, 00 ) );
        termJSON[QLatin1String( "cond" )] = static_cast<int>( SearchTerm::CondGreaterOrEqual );
        termJSON[QLatin1String( "negated" )] = true;
        subTerms << termJSON;

        termJSON[QLatin1String( "key" )] = QLatin1String( "subject" );
        termJSON[QLatin1String( "value" )] = QLatin1String( "Hello" );
        termJSON[QLatin1String( "cond" )] = static_cast<int>( SearchTerm::CondEqual );
        termJSON[QLatin1String( "negated" )] = false;
        subTerms << termJSON;

        QVariantMap map;
        map[QLatin1String( "rel" )] = static_cast<int>( SearchTerm::RelAnd );
        map[QLatin1String( "subTerms" )] = subTerms;

        ok = false;
        const QByteArray json = serializer.serialize( map, &ok );
        QVERIFY( ok );

        const SearchQuery query = SearchQuery::fromJSON( json );
        QVERIFY( !query.isNull() );
        const SearchTerm term = query.term();

        QCOMPARE( static_cast<SearchTerm::Relation>( map[QLatin1String( "rel" )].toInt() ), term.relation() );
        QCOMPARE( subTerms.count(), term.subTerms().count() );

        for ( int i = 0; i < subTerms.count(); ++i ) {
          ok = false;
          verifySimpleTerm( subTerms.at( i ).toMap(), term.subTerms()[i], &ok );
          QVERIFY( ok );
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

QTEST_AKONADIMAIN( SearchQueryTest, NoGUI )

#include "searchquerytest.moc"
