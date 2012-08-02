/*
    Copyright (c) 2012 Volker Krause <vkrause@kde.org>

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

#include <akstandarddirs.h>
#include <aktest.h>
#include "entities.h"
#include "storage/parthelper.h"

#include <QObject>
#include <QtTest/QTest>
#include <QDebug>

#define QL1S(x) QLatin1String(x)

using namespace Akonadi;

class PartHelperTest : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void testFileName()
    {
      akTestSetInstanceIdentifier( QString() );

      Part p;
      p.setId(42);

      QString fileName = PartHelper::fileNameForPart( &p );
      QVERIFY( fileName.endsWith( QL1S("/.local-unit-test/share/akonadi/file_db_data/42") ) );
      QVERIFY( fileName.startsWith( QL1S("/") ) );

      akTestSetInstanceIdentifier( QL1S("foo") );

      fileName = PartHelper::fileNameForPart( &p );
      QVERIFY( fileName.endsWith( QL1S("/.local-unit-test/share/akonadi/instance/foo/file_db_data/42") ) );
      QVERIFY( fileName.startsWith( QL1S("/") ) );
    }
};

AKTEST_MAIN( PartHelperTest )

#include "parthelpertest.moc"
