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

#include <QDebug>
#include <QDomDocument>
#include <QFile>

DbUpdater::DbUpdater(const QSqlDatabase & database, const QString & filename) :
    m_database( database ),
    m_filename( filename )
{
}

bool DbUpdater::run()
{
  QFile file( m_filename );
  if ( !file.open( QIODevice::ReadOnly ) ) {
    qWarning() << "Unable to open update description file" << m_filename;
    return false;
  }

  QDomDocument document;

  QString errorMsg;
  int line, column;
  if ( !document.setContent( &file, &errorMsg, &line, &column ) ) {
    qWarning() << "Unable to parse update description file" << m_filename << ":"
        << errorMsg << "at line" << line << "column" << column;
    return false;
  }
  const QDomElement documentElement = document.documentElement();
  if ( documentElement.tagName() != QLatin1String( "updates" ) ) {
    qWarning() << "Invalid update description file formant";
    return false;
  }

  // TODO error handling
  Akonadi::SchemaVersion currentVersion = Akonadi::SchemaVersion::retrieveAll().first();

  QDomElement updateElement = documentElement.firstChildElement();
  QMap<int, QDomElement> updates;
  while ( !updateElement.isNull() ) {
    if ( updateElement.tagName() == QLatin1String("update") ) {
      int version = updateElement.attribute( QLatin1String( "version" ), QLatin1String( "-1" ) ).toInt();
      if ( version <= 0 ) {
        qWarning() << "Invalid version attribute in database update description";
        return false;
      }
      if ( updates.contains( version ) ) {
        qWarning() << "Duplicate version attribute in database update description";
        return false;
      }
      if ( version > currentVersion.version() ) {
        updates.insert( version, updateElement );
      } else {
        qDebug() << "skipping update" << version;
      }
    }
    updateElement = updateElement.nextSiblingElement();
  }

  // QMap is sorted, so we should be replaying the changes in correct order
  for ( QMap<int,QDomElement>::ConstIterator it = updates.constBegin(); it != updates.constEnd(); ++it ) {
    Q_ASSERT( it.key() > currentVersion.version() );
    const QString sql = it.value().text().trimmed();
    bool abortOnFailure = false;
    if ( it.value().attribute( QLatin1String( "abortOnFailure" ) ) == QLatin1String( "true" ) )
      abortOnFailure = true;
    qDebug() << "DbUpdater: update to version:" << it.key() << " mandatory:" << abortOnFailure << " code:" << sql;

    bool success = m_database.transaction();
    if ( success ) {
      QSqlQuery query( m_database );
      success = query.exec( sql );
    }
    if ( success ) {
      currentVersion.setVersion( it.key() );
      success = currentVersion.update();
    }

    if ( !success || !m_database.commit() ) {
      qWarning() << "Failed to commit transaction for database update";
      m_database.rollback();
      if ( abortOnFailure )
        return false;
    }
  }

  return true;
}
