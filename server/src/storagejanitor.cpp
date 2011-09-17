/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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

#include "storagejanitor.h"

#include "storage/datastore.h"

#include <akdebug.h>

#include <QStringBuilder>
#include <QtDBus/QDBusConnection>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

using namespace Akonadi;

StorageJanitor::StorageJanitor(QObject* parent) : QThread(parent)
{
  moveToThread(this);
}

void StorageJanitor::run()
{
  DataStore::self();
  QDBusConnection con = QDBusConnection::connectToBus( QDBusConnection::SessionBus, QLatin1String(metaObject()->className()) );
  con.registerService(QLatin1String( "org.freedesktop.Akonadi.Janitor" ) );
  con.registerObject( QLatin1String( "/Janitor" ), this, QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals );
  exec();
  con.disconnectFromBus( con.name() );
  DataStore::self()->close();
}

void StorageJanitor::check()
{
  inform( "checking db not yet implemented" );
}

void StorageJanitor::vacuum()
{
  if ( DataStore::self()->database().driverName() == QLatin1String( "QMYSQL" ) ) {
    inform( "vacuuming database, that'll take some time and require a lot of temporary disk space..." );

    foreach ( const QString &table, Akonadi::allDatabaseTables() ) {
      inform( QString::fromLatin1( "optimizing table %1..." ).arg( table ) );
      const QString queryStr = QLatin1Literal( "OPTIMIZE TABLE " ) + table;
      QSqlQuery q( DataStore::self()->database() );
      if ( !q.exec( queryStr ) ) {
        akError() << "failed to optimize table" << table << ":" << q.lastError().text();
      }
    }
    inform( "vacuum done" );
  } else {
    inform( "Vacuum not supported for this database backend." );
  }
}

void StorageJanitor::inform(const char* msg)
{
  inform( QLatin1String(msg) );
}

void StorageJanitor::inform(const QString& msg)
{
  akDebug() << msg;
  emit information( msg );
}

