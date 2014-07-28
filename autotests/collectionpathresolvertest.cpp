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

#include "collectionpathresolvertest.h"
#include "collectionpathresolver.h"

#include <control.h>

#include <qtest_akonadi.h>

using namespace Akonadi;

QTEST_AKONADIMAIN( CollectionPathResolverTest )

void CollectionPathResolverTest::initTestCase()
{
  AkonadiTest::checkTestIsIsolated();
  Control::start();
}

void CollectionPathResolverTest::testPathResolver()
{
  CollectionPathResolver *resolver = new CollectionPathResolver( QLatin1String("/res1/foo/bar/bla"), this );
  AKVERIFYEXEC( resolver );
  int col = resolver->collection();
  QVERIFY( col > 0 );

  resolver = new CollectionPathResolver( Collection( col ), this );
  AKVERIFYEXEC( resolver );
  QCOMPARE( resolver->path(), QLatin1String( "res1/foo/bar/bla" ) );
}

void CollectionPathResolverTest::testRoot()
{
  CollectionPathResolver *resolver = new CollectionPathResolver( CollectionPathResolver::pathDelimiter(), this );
  AKVERIFYEXEC( resolver );
  QCOMPARE( resolver->collection(), Collection::root().id() );

  resolver = new CollectionPathResolver( Collection::root(), this );
  AKVERIFYEXEC( resolver );
  QVERIFY( resolver->path().isEmpty() );
}

void CollectionPathResolverTest::testFailure()
{
  CollectionPathResolver *resolver = new CollectionPathResolver( QLatin1String("/I/do not/exist"), this );
  QVERIFY( !resolver->exec() );

  resolver = new CollectionPathResolver( Collection( INT_MAX ), this );
  QVERIFY( !resolver->exec() );
}
