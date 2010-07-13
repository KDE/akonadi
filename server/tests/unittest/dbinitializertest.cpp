/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "dbinitializertest.h"

#include <QtCore/QDebug>
#include <QtCore/QVariant>
#include <QBuffer>

#include "storage/dbinitializer.h"

QTEST_MAIN( DbInitializerTest )

class Debugger : public DebugInterface
{
  public:
    virtual void createTableStatement( const QString &tableName, const QString &statement )
    {
      hash.insert( tableName, statement.simplified() );
    }

    QHash<QString, QString> hash;
};

void DbInitializerTest::runCreateTableStatementTest( const QString &dbIdentifier, const QString &pattern )
{
  QSqlDatabase db = QSqlDatabase::addDatabase( dbIdentifier );
  DbInitializer::Ptr initializer = DbInitializer::createInstance( db, QLatin1String(":akonadidb.xml") );

  Debugger *debugger = new Debugger;
  initializer->setDebugInterface( debugger );
  initializer->run();

  QHashIterator<QString, QString> it( debugger->hash );
  while ( it.hasNext() ) {
    it.next();

    const QString fileName = pattern + it.key().toLower().remove( QLatin1String( "table" ) );
    QFile file( fileName );

    if ( !file.open( QIODevice::ReadOnly ) ) {
      QVERIFY( false );
      return;
    }

    const QString data = QString::fromUtf8( file.readAll() ).simplified();
    QString output = it.value();
    output.replace( " ,", "," );
    output.replace( " )", ")" );
    QCOMPARE( output, data );
  }

  delete debugger;
  QSqlDatabase::removeDatabase( db.databaseName() );
}

void DbInitializerTest::testMysqlCreateTableStatement()
{
  runCreateTableStatementTest( QLatin1String( "QMYSQL" ), QLatin1String( ":mysql_ct_" ) );
}

void DbInitializerTest::testPsqlCreateTableStatement()
{
  runCreateTableStatementTest( QLatin1String( "QPSQL" ), QLatin1String( ":psql_ct_" ) );
}

void DbInitializerTest::testSqliteCreateTableStatement()
{
  runCreateTableStatementTest( QLatin1String( "QSQLITE" ), QLatin1String( ":sqlite_ct_" ) );
}

void DbInitializerTest::testOdbcCreateTableStatement()
{
  runCreateTableStatementTest( QLatin1String( "QODBC" ), QLatin1String( ":odbc_ct_" ) );
}

#include "dbinitializertest.moc"
