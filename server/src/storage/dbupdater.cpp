/*
    Copyright (c) 2007 - 2012 Volker Krause <vkrause@kde.org>

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

#include "dbupdater.h"
#include "dbtype.h"
#include "entities.h"
#include "akdebug.h"
#include "akdbus.h"
#include "querybuilder.h"
#include "selectquerybuilder.h"


#include <QCoreApplication>
#include <QMetaMethod>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QThread>

#include <QDomDocument>
#include <QFile>
#include <QtSql/qsqlresult.h>

DbUpdater::DbUpdater( const QSqlDatabase & database, const QString & filename )
  : m_database( database ),
    m_filename( filename )
{
}

bool DbUpdater::run()
{
  Q_ASSERT( QThread::currentThread() == QCoreApplication::instance()->thread() );

  // TODO error handling
  Akonadi::SchemaVersion currentVersion = Akonadi::SchemaVersion::retrieveAll().first();

  UpdateSet::Map updates;

  if ( !parseUpdateSets( currentVersion.version(), updates ) )
    return false;

  if ( updates.isEmpty() )
    return true;
  // indicate clients this might take a while
  // we can ignore unregistration in error cases, that'll kill the server anyway
  if ( !QDBusConnection::sessionBus().registerService( AkDBus::serviceName( AkDBus::UpgradeIndicator ) ) ) {
    akFatal() << "Unable to connect to dbus service: " << QDBusConnection::sessionBus().lastError().message();
  }

  // QMap is sorted, so we should be replaying the changes in correct order
  for ( QMap<int, UpdateSet>::ConstIterator it = updates.constBegin(); it != updates.constEnd(); ++it ) {
    Q_ASSERT( it.key() > currentVersion.version() );
    akDebug() << "DbUpdater: update to version:" << it.key() << " mandatory:" << it.value().abortOnFailure;

    bool success = m_database.transaction();
    if ( success ) {
      if ( it.value().complex ) {
        const QString methodName = QString::fromLatin1( "complexUpdate_%1()" ).arg( it.value().version );
        const int index = metaObject()->indexOfMethod( methodName.toLatin1().constData() );
        if ( index == -1 ) {
          success = false;
          akError() << "Update to version" << it.value().version << "marked as complex, but no implementation is available";
        } else {
          const QMetaMethod method = metaObject()->method( index );
          method.invoke( this, Q_RETURN_ARG(bool, success) );
        }
      } else {
        Q_FOREACH ( const QString &statement, it.value().statements ) {
          QSqlQuery query( m_database );
          success = query.exec( statement );
          if ( !success ) {
            akError() << "DBUpdater: query error:" << query.lastError().text() << m_database.lastError().text();
            akError() << "Query was: " << statement;
            akError() << "Target version was: " << it.key();
            akError() << "Mandatory: " << it.value().abortOnFailure;
          }
        }
      }
    }
    if ( success ) {
      currentVersion.setVersion( it.key() );
      success = currentVersion.update();
    }

    if ( !success || !m_database.commit() ) {
      akError() << "Failed to commit transaction for database update";
      m_database.rollback();
      if ( it.value().abortOnFailure )
        return false;
    }
  }

  QDBusConnection::sessionBus().unregisterService( AkDBus::serviceName(AkDBus::UpgradeIndicator) );
  return true;
}

bool DbUpdater::parseUpdateSets( int currentVersion, UpdateSet::Map &updates ) const
{
  QFile file( m_filename );
  if ( !file.open( QIODevice::ReadOnly ) ) {
    akError() << "Unable to open update description file" << m_filename;
    return false;
  }

  QDomDocument document;

  QString errorMsg;
  int line, column;
  if ( !document.setContent( &file, &errorMsg, &line, &column ) ) {
    akError() << "Unable to parse update description file" << m_filename << ":"
        << errorMsg << "at line" << line << "column" << column;
    return false;
  }

  const QDomElement documentElement = document.documentElement();
  if ( documentElement.tagName() != QLatin1String( "updates" ) ) {
    akError() << "Invalid update description file formant";
    return false;
  }

  // iterate over the xml document and extract update information into an UpdateSet
  QDomElement updateElement = documentElement.firstChildElement();
  while ( !updateElement.isNull() ) {
    if ( updateElement.tagName() == QLatin1String( "update" ) ) {
      const int version = updateElement.attribute( QLatin1String( "version" ), QLatin1String( "-1" ) ).toInt();
      if ( version <= 0 ) {
        akError() << "Invalid version attribute in database update description";
        return false;
      }

      if ( updates.contains( version ) ) {
        akError() << "Duplicate version attribute in database update description";
        return false;
      }

      if ( version <= currentVersion ) {
        akDebug() << "skipping update" << version;
      } else {
        UpdateSet updateSet;
        updateSet.version = version;
        updateSet.abortOnFailure = (updateElement.attribute( QLatin1String( "abortOnFailure" ) ) == QLatin1String( "true" ));

        QDomElement childElement = updateElement.firstChildElement();
        while ( !childElement.isNull() ) {
          if ( childElement.tagName() == QLatin1String( "raw-sql" ) ) {
            if ( updateApplicable( childElement.attribute( QLatin1String( "backends" ) ) ) ) {
              updateSet.statements << buildRawSqlStatement( childElement );
            }
          } else if ( childElement.tagName() == QLatin1String( "complex-update" ) ) {
            if ( updateApplicable( childElement.attribute( QLatin1String( "backends" ) ) ) ) {
              updateSet.complex = true;
            }
          }
          //TODO: check for generic tags here in the future

          childElement = childElement.nextSiblingElement();
        }

        updates.insert( version, updateSet );
      }
    }
    updateElement = updateElement.nextSiblingElement();
  }

  return true;
}

bool DbUpdater::updateApplicable( const QString &backends ) const
{
  const QStringList matchingBackends = backends.split( QLatin1Char( ',' ) );

  QString currentBackend;
  switch ( DbType::type( m_database ) ) {
    case DbType::MySQL:
      currentBackend = QLatin1String( "mysql" ); break;
    case DbType::PostgreSQL:
      currentBackend = QLatin1String( "psql" ); break;
    case DbType::Sqlite:
      currentBackend = QLatin1String( "sqlite" ); break;
    case DbType::Virtuoso:
      currentBackend = QLatin1String( "odbc" ); break;
    case DbType::Unknown:
      return false;
  }

  return matchingBackends.contains( currentBackend );
}

QString DbUpdater::buildRawSqlStatement( const QDomElement &element ) const
{
  return element.text().trimmed();
}

bool DbUpdater::complexUpdate_25()
{
  akDebug() << "Starting update to version 25";

  // Get list of all part names
  Akonadi::QueryBuilder qb( QLatin1String( "PartTable" ), Akonadi::QueryBuilder::Select );
  qb.setDistinct( true );
  qb.addColumn( QLatin1String( "PartTable.name" ) );

  if ( !qb.exec() ) {
    akError() << qb.query().lastError().text();
    return false;
  }

  // Process them one by one
  QSqlQuery query = qb.query();
  while ( query.next() ) {
    const QString partName = query.value( 0 ).toString();
    const QString ns = partName.left( 3 );
    const QString name = partName.mid( 4 );
    akDebug() << "Moving part type" << partName << "to PartTypeTable";

    // Split the part name to namespace and name and insert it to PartTypeTable
    Akonadi::QueryBuilder qb2( QLatin1String( "PartTypeTable" ), Akonadi::QueryBuilder::Insert );
    qb2.setColumnValue( QLatin1String( "ns" ), ns );
    qb2.setColumnValue( QLatin1String( "name" ), name );
    if ( !qb2.exec() ) {
      akError() << qb.query().lastError().text();
      return false;
    }
    const qint64 id = qb2.insertId();

    akDebug() << "Updating" << partName << "entries in PartTable.partTypeId to" << id;
    // Store id of the part type in PartTable partTypeId column
    Akonadi::QueryBuilder qb3( QLatin1String( "PartTable" ), Akonadi::QueryBuilder::Update );
    qb3.setColumnValue( QLatin1String( "partTypeId" ), id );
    qb3.addValueCondition( QLatin1String( "name" ), Akonadi::Query::Equals, partName );
    if ( !qb3.exec() ) {
      akError() << qb.query().lastError().text();
      return false;
    }
  }

  akDebug() << "Done.";
  akDebug() << "Removing PartTable.partType column now";

  // Ok done. Next raw-sql element will take care of dropping the name column from PartTable
  return true;
}


#include "dbupdater.moc"
