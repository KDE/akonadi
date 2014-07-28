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

#include "test_utils.h"

#include <control.h>
#include <collection.h>
#include <collectionfetchjob.h>
#include <collectionfetchscope.h>
#include <subscriptionjob_p.h>

#include <QtCore/QObject>

#include <qtest_akonadi.h>

using namespace Akonadi;

class SubscriptionTest : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void initTestCase()
    {
      AkonadiTest::checkTestIsIsolated();
      Control::start();
    }

    void testSubscribe()
    {
      Collection::List l;
      l << Collection( collectionIdFromPath( QLatin1String("res2/foo2") ) );
      QVERIFY( l.first().isValid() );
      SubscriptionJob *sjob = new SubscriptionJob( this );
      sjob->unsubscribe( l );
      AKVERIFYEXEC( sjob );

      const Collection res2Col = Collection( collectionIdFromPath( QLatin1String("res2") ) );
      QVERIFY( res2Col.isValid() );
      CollectionFetchJob *ljob = new CollectionFetchJob( res2Col, CollectionFetchJob::FirstLevel, this );
      AKVERIFYEXEC( ljob );
      QCOMPARE( ljob->collections().count(), 1 );

      ljob = new CollectionFetchJob( res2Col, CollectionFetchJob::FirstLevel, this );
      ljob->fetchScope().setIncludeUnsubscribed( true );
      AKVERIFYEXEC( ljob );
      QCOMPARE( ljob->collections().count(), 2 );

      sjob = new SubscriptionJob( this );
      sjob->subscribe( l );
      AKVERIFYEXEC( sjob );

      ljob = new CollectionFetchJob( res2Col, CollectionFetchJob::FirstLevel, this );
      AKVERIFYEXEC( ljob );
      QCOMPARE( ljob->collections().count(), 2 );
    }

    void testEmptySubscribe()
    {
      Collection::List l;
      SubscriptionJob *sjob = new SubscriptionJob( this );
      AKVERIFYEXEC( sjob );
    }

    void testInvalidSubscribe()
    {
      Collection::List l;
      l << Collection( 1 );
      SubscriptionJob *sjob = new SubscriptionJob( this );
      sjob->subscribe( l );
      l << Collection( INT_MAX );
      sjob->unsubscribe( l );
      QVERIFY( !sjob->exec() );
    }
};

QTEST_AKONADIMAIN( SubscriptionTest )

#include "subscriptiontest.moc"
