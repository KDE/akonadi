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

#include <libakonadi/control.h>
#include <libakonadi/collection.h>
#include <libakonadi/collectionlistjob.h>
#include <libakonadi/subscriptionjob.h>

#include <QtCore/QObject>

#include <qtest_kde.h>

using namespace Akonadi;

class SubscriptionTest : public QObject
{
  Q_OBJECT
  private slots:
    void initTestCase()
    {
      Control::start();
    }

    void testSubscribe()
    {
      Collection::List l;
      l << Collection( 5 );
      SubscriptionJob *sjob = new SubscriptionJob( this );
      sjob->unsubscribe( l );
      QVERIFY( sjob->exec() );

      CollectionListJob *ljob = new CollectionListJob( Collection( 7 ), CollectionListJob::Flat, this );
      QVERIFY( ljob->exec() );
      QCOMPARE( ljob->collections().count(), 1 );

      ljob = new CollectionListJob( Collection( 7 ), CollectionListJob::Flat, this );
      ljob->includeUnsubscribed();
      QVERIFY( ljob->exec() );
      QCOMPARE( ljob->collections().count(), 2 );

      sjob = new SubscriptionJob( this );
      sjob->subscribe( l );
      QVERIFY( sjob->exec() );

      ljob = new CollectionListJob( Collection( 7 ), CollectionListJob::Flat, this );
      QVERIFY( ljob->exec() );
      QCOMPARE( ljob->collections().count(), 2 );
    }

    void testEmptySubscribe()
    {
      Collection::List l;
      SubscriptionJob *sjob = new SubscriptionJob( this );
      QVERIFY( sjob->exec() );
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

QTEST_KDEMAIN( SubscriptionTest, NoGUI )

#include "subscriptiontest.moc"
