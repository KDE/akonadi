/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include <libakonadi/control.h>
#include <libakonadi/collectioncopyjob.h>
#include <libakonadi/collectionlistjob.h>

#include <QtCore/QObject>

#include <qtest_kde.h>

using namespace Akonadi;

class CollectionCopyTest : public QObject
{
  Q_OBJECT
  private slots:
    void initTestCase()
    {
      Control::start();
    }

    void testCopy()
    {
      const Collection target( 8 );
      const Collection source( 10 );

      CollectionCopyJob *copy = new CollectionCopyJob( source, target );
      QVERIFY( copy->exec() );

      CollectionListJob *list = new CollectionListJob( target, CollectionListJob::Recursive );
      QVERIFY( list->exec() );
      QCOMPARE( list->collections().count(), 4 );

      Collection copied = list->collections().first();
      QVERIFY( copied.remoteId().isEmpty() );
      QCOMPARE( copied.resource(), QString("akonadi_dummy_resource_3") );
    }

    void testIlleagalCopy()
    {
      // invalid source
      CollectionCopyJob* copy = new CollectionCopyJob( Collection(), Collection( 1 ) );
      QVERIFY( !copy->exec() );

      // non-existing source
      copy = new CollectionCopyJob( Collection( INT_MAX ), Collection( 1 ) );
      QVERIFY( !copy->exec() );

      // invalid target
      copy = new CollectionCopyJob( Collection( 1 ), Collection() );
      QVERIFY( !copy->exec() );

      // non-existing target
      copy = new CollectionCopyJob( Collection( 1 ), Collection( INT_MAX ) );
      QVERIFY( !copy->exec() );
    }

};

QTEST_KDEMAIN( CollectionCopyTest, NoGUI )

#include "collectioncopytest.moc"
