/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "searchjobtest.h"
#include "searchjobtest.moc"

#include <libakonadi/collection.h>
#include <libakonadi/collectiondeletejob.h>
#include <libakonadi/collectionlistjob.h>
#include <libakonadi/searchcreatejob.h>

#include <qtest_kde.h>

QTEST_KDEMAIN( SearchJobTest, NoGUI )

using namespace Akonadi;

void SearchJobTest::testCreateDeleteSearch()
{
  SearchCreateJob *create = new SearchCreateJob( "search123456", "query", this );
  QVERIFY( create->exec() );

  CollectionListJob *list = new CollectionListJob( Collection( 1 ), CollectionListJob::Recursive, this );
  QVERIFY( list->exec() );
  Collection::List cols = list->collections();
  Collection col;
  foreach ( const Collection c, cols ) {
    if ( c.name() == "search123456" )
      col = c;
  }
  QVERIFY( col.isValid() );
  QCOMPARE( col.parent(), 1 );
  QCOMPARE( col.type(), Collection::Virtual );

  CollectionDeleteJob *delJob = new CollectionDeleteJob( col, this );
  QVERIFY( delJob->exec() );
}
