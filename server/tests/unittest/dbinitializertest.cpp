/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>
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

#include "dbinitializertest.h"

#include <QtCore/QDebug>
#include <QtCore/QVariant>

#include "storage/dbinitializer.h"
#include <aktest.h>

AKTEST_MAIN( DbInitializerTest )

class StatementCollector : public TestInterface
{
  public:
    virtual void execStatement(const QString& statement)
    {
      statements.push_back(statement);
    }

    QStringList statements;
};

class DbFakeIntrospector : public DbIntrospector
{
  public:
    DbFakeIntrospector(const QSqlDatabase& database) : DbIntrospector(database) {}
    virtual bool hasTable(const QString& tableName)
    {
      Q_UNUSED( tableName );
      return false;
    }
    virtual bool hasIndex(const QString& tableName, const QString& indexName)
    {
      Q_UNUSED( tableName );
      Q_UNUSED( indexName );
      return false;
    }
};

void DbInitializerTest::initTestCase()
{
  Q_INIT_RESOURCE( akonadidb );
}

void DbInitializerTest::testRun_data()
{
  QTest::addColumn<QString>( "driverName" );
  QTest::addColumn<QString>( "filename" );

  QTest::newRow( "mysql" ) << "QMYSQL" << ":dbinit_mysql";
  QTest::newRow( "sqlite" ) << "QSQLITE" << ":dbinit_sqlite";
  QTest::newRow( "psql" ) << "QPSQL" << ":dbinit_psql";
  QTest::newRow( "virtuoso" ) << "QODBC" << ":dbinit_odbc";
}

void DbInitializerTest::testRun()
{
  QFETCH( QString, driverName );
  QFETCH( QString, filename );

  QFile file( filename );
  QVERIFY( file.open( QFile::ReadOnly ) );

  if ( QSqlDatabase::drivers().contains( driverName ) ) {
    QSqlDatabase db = QSqlDatabase::addDatabase( driverName, driverName );
    DbInitializer::Ptr initializer = DbInitializer::createInstance( db, QLatin1String(":akonadidb.xml") );
    QVERIFY( initializer );

    StatementCollector collector;
    initializer->setTestInterface( &collector );
    DbIntrospector::Ptr introspector( new DbFakeIntrospector( db ) );
    initializer->setIntrospector( introspector );

    QVERIFY( initializer->run() );
    QVERIFY( !collector.statements.isEmpty() );

    Q_FOREACH ( const QString &statement, collector.statements ) {
      const QString expected = readNextStatement( &file ).simplified();

      QString normalized = statement.simplified();
      normalized.replace( QLatin1String(" ,"), QLatin1String(",") );
      normalized.replace( QLatin1String(" )"), QLatin1String(")") );
      QCOMPARE( normalized, expected );
    }

  }
}

QString DbInitializerTest::readNextStatement(QIODevice* io)
{
  QString statement;
  while ( !io->atEnd() ) {
    const QString line = QString::fromUtf8( io->readLine() );
    if ( line.trimmed().isEmpty() && !statement.isEmpty() )
      return statement;
    statement += line;
  }

  return statement;
}

#include "dbinitializertest.moc"
