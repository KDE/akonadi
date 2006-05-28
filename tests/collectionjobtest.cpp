/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "collectionjobtest.h"
#include <qtest_kde.h>

#include "collection.h"
#include "collectionlistjob.h"

using namespace PIM;

QTEST_KDEMAIN( CollectionJobTest, NoGUI );

static Collection* findCol( const Collection::List &list, const QByteArray &path ) {
  foreach ( Collection* col, list )
    if ( col->path() == path )
      return col;
  return 0;
}

 // ### These tests rely on an running akonadi server which uses the test database!!
void CollectionJobTest::testTopLevelList( )
{
  // non-recursive top-level list
  CollectionListJob *job = new CollectionListJob( Collection::root() );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();
  delete job;

  // check if everything is there and has the correct types and attributes
  QCOMPARE( list.count(), 4 );
  Collection *col;

  col = findCol( list, "res1" );
  QVERIFY( col != 0 );
  QCOMPARE( col->type(), Collection::Resource );

  QVERIFY( findCol( list, "res2" ) != 0 );
  QVERIFY( findCol( list, "res3" ) != 0 );

  col = findCol( list, "Search" );
  QVERIFY( col != 0 );
  QCOMPARE( col->type(), Collection::VirtualParent );
}

#include "collectionjobtest.moc"
