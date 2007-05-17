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
#include "collectionpathresolvertest.moc"

#include <libakonadi/collectionpathresolver.h>
#include <libakonadi/control.h>

#include <qtest_kde.h>

using namespace Akonadi;

QTEST_KDEMAIN( CollectionPathResolverTest, NoGUI )

void CollectionPathResolverTest::initTestCase()
{
  Control::start();
}

void CollectionPathResolverTest::testPathResolver()
{
  CollectionPathResolver *resolver = new CollectionPathResolver( "/res1/foo/bar/bla", this );
  QVERIFY( resolver->exec() );
  int col = resolver->collection();
  QVERIFY( col > 0 );

  resolver = new CollectionPathResolver( Collection( col ), this );
  QVERIFY( resolver->exec() );
  QCOMPARE( resolver->path(), QString( "res1/foo/bar/bla" ) );
}

void CollectionPathResolverTest::testRoot()
{
  CollectionPathResolver *resolver = new CollectionPathResolver( Collection::delimiter(), this );
  QVERIFY( resolver->exec() );
  QCOMPARE( resolver->collection(), Collection::root().id() );

  resolver = new CollectionPathResolver( Collection::root(), this );
  QVERIFY( resolver->exec() );
  QVERIFY( resolver->path().isEmpty() );
}

void CollectionPathResolverTest::testFailure()
{
  CollectionPathResolver *resolver = new CollectionPathResolver( "/I/do not/exist", this );
  QVERIFY( !resolver->exec() );

  resolver = new CollectionPathResolver( Collection( INT_MAX ), this );
  QVERIFY( !resolver->exec() );
}
