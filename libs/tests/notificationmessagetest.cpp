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

#include "notificationmessagetest.h"
#include "notificationmessagetest.moc"

#include <libakonadi/notificationmessage.h>

#include <qtest_kde.h>

QTEST_KDEMAIN( NotificationMessageTest, NoGUI )

using namespace Akonadi;

void NotificationMessageTest::testCompress()
{
  NotificationMessage::List list;
  NotificationMessage msg;
  msg.setType( NotificationMessage::Item );
  msg.setOperation( NotificationMessage::Add );

  NotificationMessage::appendAndCompress( list, msg );
  QCOMPARE( list.count(), 1 );

  msg.setOperation( NotificationMessage::Modify );
  NotificationMessage::appendAndCompress( list, msg );
  QCOMPARE( list.count(), 1 );
  QCOMPARE( list.first().operation(), NotificationMessage::Add );

  msg.setOperation( NotificationMessage::Remove );
  NotificationMessage::appendAndCompress( list, msg );
  QCOMPARE( list.count(), 2 ); // should be 2 for collections, 0 for items?
}

void NotificationMessageTest::testCompress2()
{
  NotificationMessage::List list;
  NotificationMessage msg;
  msg.setType( NotificationMessage::Item );
  msg.setOperation( NotificationMessage::Modify );

  NotificationMessage::appendAndCompress( list, msg );
  QCOMPARE( list.count(), 1 );

  msg.setOperation( NotificationMessage::Remove );
  NotificationMessage::appendAndCompress( list, msg );
  QCOMPARE( list.count(), 1 );
  QCOMPARE( list.first().operation(), NotificationMessage::Remove );
}

void NotificationMessageTest::testCompress3()
{
  NotificationMessage::List list;
  NotificationMessage msg;
  msg.setType( NotificationMessage::Item );
  msg.setOperation( NotificationMessage::Modify );

  NotificationMessage::appendAndCompress( list, msg );
  QCOMPARE( list.count(), 1 );

  NotificationMessage::appendAndCompress( list, msg );
  QCOMPARE( list.count(), 1 );
}

void NotificationMessageTest::testNoCompress()
{
  NotificationMessage::List list;
  NotificationMessage msg;
  msg.setType( NotificationMessage::Item );
  msg.setOperation( NotificationMessage::Modify );

  NotificationMessage::appendAndCompress( list, msg );
  QCOMPARE( list.count(), 1 );

  msg.setType( NotificationMessage::Collection );
  NotificationMessage::appendAndCompress( list, msg );
  QCOMPARE( list.count(), 2 );
}
