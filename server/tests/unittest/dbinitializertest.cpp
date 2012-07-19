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
    DbFakeIntrospector(const QSqlDatabase& database) : DbIntrospector(database), m_hasTable(false), m_hasIndex(false), m_tableEmpty(true) {}
    virtual bool hasTable(const QString& tableName)
    {
      Q_UNUSED( tableName );
      return m_hasTable;
    }
    virtual bool hasIndex(const QString& tableName, const QString& indexName)
    {
      Q_UNUSED( tableName );
      Q_UNUSED( indexName );
      return m_hasIndex;
    }
    virtual bool hasColumn(const QString& tableName, const QString& columnName)
    {
      return false;
    }
    virtual bool isTableEmpty(const QString& tableName)
    {
      Q_UNUSED( tableName );
      return m_tableEmpty;
    }

    bool m_hasTable;
    bool m_hasIndex;
    bool m_tableEmpty;
};

void DbInitializerTest::initTestCase()
{
  Q_INIT_RESOURCE( akonadidb );
}

void DbInitializerTest::testRun_data()
{
  QTest::addColumn<QString>( "driverName" );
  QTest::addColumn<QString>( "filename" );
  QTest::addColumn<bool>( "hasTable" );

  QTest::newRow( "mysql" ) << "QMYSQL" << ":dbinit_mysql" << false;
  QTest::newRow( "sqlite" ) << "QSQLITE" << ":dbinit_sqlite" << false;
  QTest::newRow( "psql" ) << "QPSQL" << ":dbinit_psql" << false;
  QTest::newRow( "virtuoso" ) << "QODBC" << ":dbinit_odbc" << false;

  QTest::newRow( "mysql" ) << "QMYSQL" << ":dbinit_mysql_incremental" << true;
  QTest::newRow( "sqlite" ) << "QSQLITE" << ":dbinit_sqlite_incremental" << true;
  QTest::newRow( "psql" ) << "QPSQL" << ":dbinit_psql_incremental" << true;
  QTest::newRow( "virtuoso" ) << "QODBC" << ":dbinit_odbc_incremental" << true;
}

void DbInitializerTest::testRun()
{
  QFETCH( QString, driverName );
  QFETCH( QString, filename );
  QFETCH( bool, hasTable );

  QFile file( filename );
  QVERIFY( file.open( QFile::ReadOnly ) );

  if ( QSqlDatabase::drivers().contains( driverName ) ) {
    QSqlDatabase db = QSqlDatabase::addDatabase( driverName, driverName );
    DbInitializer::Ptr initializer = DbInitializer::createInstance( db, QLatin1String(":akonadidb.xml") );
    QVERIFY( initializer );

    StatementCollector collector;
    initializer->setTestInterface( &collector );
    DbFakeIntrospector* introspector = new DbFakeIntrospector( db );
    introspector->m_hasTable = hasTable;
    introspector->m_hasIndex = hasTable;
    introspector->m_tableEmpty = !hasTable;
    initializer->setIntrospector( DbIntrospector::Ptr( introspector ) );

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
