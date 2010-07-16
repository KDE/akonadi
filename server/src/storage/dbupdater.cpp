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

#include "dbupdater.h"
#include "entities.h"
#include "akdebug.h"

#include <QSqlQuery>
#include <QSqlError>

#include <QDomDocument>
#include <QFile>

DbUpdater::DbUpdater( const QSqlDatabase & database, const QString & filename )
  : m_database( database ),
    m_filename( filename )
{
}

bool DbUpdater::run()
{
  // TODO error handling
  Akonadi::SchemaVersion currentVersion = Akonadi::SchemaVersion::retrieveAll().first();

  UpdateSet::Map updates;

  if ( !parseUpdateSets( currentVersion.version(), updates ) )
    return false;

  // QMap is sorted, so we should be replaying the changes in correct order
  for ( QMap<int, UpdateSet>::ConstIterator it = updates.constBegin(); it != updates.constEnd(); ++it ) {
    Q_ASSERT( it.key() > currentVersion.version() );
    akDebug() << "DbUpdater: update to version:" << it.key() << " mandatory:" << it.value().abortOnFailure;

    bool success = m_database.transaction();
    if ( success ) {
      foreach ( const QString &statement, it.value().statements ) {
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
  if ( m_database.driverName() == QLatin1String( "QMYSQL" ) )
    currentBackend = QLatin1String( "mysql" );
  else if ( m_database.driverName() == QLatin1String( "QPSQL" ) )
    currentBackend = QLatin1String( "psql" );
  else if ( m_database.driverName() == QLatin1String( "QSQLITE" ) )
    currentBackend = QLatin1String( "sqlite" );
  else if ( m_database.driverName() == QLatin1String( "QODBC" ) )
    currentBackend = QLatin1String( "odbc" );

  return matchingBackends.contains( currentBackend );
}

QString DbUpdater::buildRawSqlStatement( const QDomElement &element ) const
{
  return element.text().trimmed();
}
