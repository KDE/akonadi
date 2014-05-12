/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>
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

#ifndef AKTEST_H
#define AKTEST_H

#include "akapplication.h"
#include <QDebug>
#include <QBuffer>
#include <QTest>

#define AKTEST_MAIN( TestObject ) \
int main( int argc, char **argv ) \
{ \
  qputenv( "XDG_DATA_HOME", ".local-unit-test/share" ); \
  qputenv( "XDG_CONFIG_HOME", ".config-unit-test" ); \
  AkCoreApplication app( argc, argv ); \
  app.parseCommandLine(); \
  TestObject tc; \
  return QTest::qExec( &tc, argc, argv ); \
}

#define AKTEST_FAKESERVER_MAIN(TestObject) \
int main(int argc, char **argv) \
{ \
  AkCoreApplication app(argc, argv); \
  app.parseCommandLine(); \
  TestObject tc; \
  return QTest::qExec(&tc, argc, argv); \
}

// Takes from Qt 5
#if QT_VERSION < 0x050000

// Will try to wait for the expression to become true while allowing event processing
#define QTRY_VERIFY(__expr) \
do { \
    const int __step = 50; \
    const int __timeout = 5000; \
    if ( !( __expr ) ) { \
        QTest::qWait( 0 ); \
    } \
    for ( int __i = 0; __i < __timeout && !( __expr ); __i += __step ) { \
        QTest::qWait( __step ); \
    } \
    QVERIFY( __expr ); \
} while ( 0 )

// Will try to wait for the comparison to become successful while allowing event processing
#define QTRY_COMPARE(__expr, __expected) \
do { \
    const int __step = 50; \
    const int __timeout = 5000; \
    if ( ( __expr ) != ( __expected ) ) { \
        QTest::qWait( 0 ); \
    } \
    for ( int __i = 0; __i < __timeout && ( ( __expr ) != ( __expected ) ); __i += __step ) { \
        QTest::qWait( __step ); \
    } \
    QCOMPARE( __expr, __expected ); \
} while ( 0 )

#endif



inline void akTestSetInstanceIdentifier( const QString &instanceId )
{
  AkApplication::setInstanceIdentifier( instanceId );
}

#include <libs/notificationmessagev3_p.h>

namespace QTest {
  template<>
  char *toString(const Akonadi::NotificationMessageV3 &msg)
  {
    QByteArray ba;
    QBuffer buf;
    buf.setBuffer(&ba);
    buf.open(QIODevice::WriteOnly);
    QDebug dbg(&buf);
    dbg.nospace() << msg;
    buf.close();
    return qstrdup(ba.constData());
  }
}

#endif
