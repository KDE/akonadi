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
#include "storage/selectquerybuilder.h"

#include <akdebug.h>
#include <libs/protocol_p.h>

#include <QStringBuilder>
#include <QtDBus/QDBusConnection>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include <boost/bind.hpp>
#include <algorithm>

using namespace Akonadi;

StorageJanitor::StorageJanitor(QObject* parent) : QThread(parent)
{
  moveToThread(this);
}

void StorageJanitor::run()
{
  DataStore::self();
  QDBusConnection con = QDBusConnection::connectToBus( QDBusConnection::SessionBus, QLatin1String(metaObject()->className()) );
  con.registerService(QLatin1String( AKONADI_DBUS_STORAGEJANITOR_SERVICE ) );
  con.registerObject( QLatin1String( AKONADI_DBUS_STORAGEJANITOR_PATH ), this, QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals );
  exec();
  con.disconnectFromBus( con.name() );
  DataStore::self()->close();
}

void StorageJanitor::check()
{
  inform( "Looking for collections not belonging to a valid resource..." );
  findOrphanedCollections();

  inform( "Checking collection tree consistency..." );
  const Collection::List cols = Collection::retrieveAll();
  std::for_each( cols.begin(), cols.end(), boost::bind( &StorageJanitor::checkPathToRoot, this, _1 ) );

  inform( "Looking for items not belonging to a valid collection..." );
  findOrphanedItems();

  inform( "Looking for item parts not belonging to a valid item..." );
  findOrphanedParts();

  inform( "Looking for overlapping external parts..." );
  findOverlappingParts();

  /* TODO some ideas for further checks:
   * the collection tree is non-cyclic
   * every item payload part belongs to an existing item
   * every part points to an existing file
   * every payload file belongs to an existing part object
   * content type constraints of collections are not violated
   * look for dirty/RID-less items
   * find unused flags
   * find unused mimetypes
   * check for dead entries in relation tables
   */

  inform( "Consistency check done." );
}

void StorageJanitor::findOrphanedCollections()
{
  SelectQueryBuilder<Collection> qb;
  qb.addJoin( QueryBuilder::LeftJoin, Resource::tableName(), Collection::resourceIdFullColumnName(), Resource::idFullColumnName() );
  qb.addValueCondition( Resource::idFullColumnName(), Query::Is, QVariant() );

  qb.exec();
  const Collection::List orphans = qb.result();
  if ( orphans.size() > 0 ) {
    inform( QLatin1Literal( "Found " ) + QString::number( orphans.size() ) + QLatin1Literal( " orphan collections." ) );
    // TODO: attach to lost+found resource
  }
}

void StorageJanitor::checkPathToRoot(const Akonadi::Collection& col)
{
  if ( col.parentId() == 0 )
    return;
  const Akonadi::Collection parent = col.parent();
  if ( !parent.isValid() ) {
    inform( QLatin1Literal( "Collection \"" ) + col.name() + QLatin1Literal( "\" (id: " ) + QString::number( col.id()  )
          + QLatin1Literal( ") has no valid parent." ) );
    // TODO fix that by attaching to a top-level lost+found folder
    return;
  }

  if ( col.resourceId() != parent.resourceId() ) {
    inform( QLatin1Literal( "Collection \"" ) + col.name() + QLatin1Literal( "\" (id: " ) + QString::number( col.id()  )
          + QLatin1Literal( ") belongs to a different resource than its parent." ) );
    // can/should we actually fix that?
  }
  
  checkPathToRoot( parent );
}

void StorageJanitor::findOrphanedItems()
{
  SelectQueryBuilder<PimItem> qb;
  qb.addJoin( QueryBuilder::LeftJoin, Collection::tableName(), PimItem::collectionIdFullColumnName(), Collection::idFullColumnName() );
  qb.addValueCondition( Collection::idFullColumnName(), Query::Is, QVariant() );

  qb.exec();
  const PimItem::List orphans = qb.result();
  if ( orphans.size() > 0 ) {
    inform( QLatin1Literal( "Found " ) + QString::number( orphans.size() ) + QLatin1Literal( " orphan items." ) );
    // TODO: attach to lost+found collection
  }
}

void StorageJanitor::findOrphanedParts()
{
  SelectQueryBuilder<Part> qb;
  qb.addJoin( QueryBuilder::LeftJoin, PimItem::tableName(), Part::pimItemIdFullColumnName(), PimItem::idFullColumnName() );
  qb.addValueCondition( PimItem::idFullColumnName(), Query::Is, QVariant() );

  qb.exec();
  const Part::List orphans = qb.result();
  if ( orphans.size() > 0 ) {
    inform( QLatin1Literal( "Found " ) + QString::number( orphans.size() ) + QLatin1Literal( " orphan items." ) );
    // TODO: create lost+found items for those? delete?
  }
}

void StorageJanitor::findOverlappingParts()
{
  QueryBuilder qb( Part::tableName(), QueryBuilder::Select );
  qb.addColumn( Part::dataColumn() );
  qb.addColumn( QLatin1Literal("count(") + Part::idColumn() + QLatin1Literal(") as cnt") );
  qb.addValueCondition( Part::externalColumn(), Query::Equals, true );
  qb.addValueCondition( Part::dataColumn(), Query::IsNot, QVariant() );
  qb.addGroupColumn( Part::dataColumn() );
  qb.addValueCondition( QLatin1String("cnt"), Query::Greater, 1, QueryBuilder::HavingCondition );
  qb.exec();

  int count = 0;
  while ( qb.query().next() ) {
    ++count;
    inform( QLatin1Literal( "Found overlapping item: " ) + qb.query().value( 0 ).toString() );
    // TODO: uh oh, this is bad, how do we recover from that?
  }

  if ( count > 0 )
    inform( QLatin1Literal( "Found " ) + QString::number( count ) + QLatin1Literal( " overlapping parts - bad." ) );
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

