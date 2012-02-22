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

#include <akdbus.h>
#include <akdebug.h>
#include <akstandarddirs.h>

#include <libs/protocol_p.h>
#include <libs/xdgbasedirs_p.h>

#include <QStringBuilder>
#include <QtDBus/QDBusConnection>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QDir>
#include <QtCore/qdiriterator.h>

#include <boost/bind.hpp>
#include <algorithm>

using namespace Akonadi;

StorageJanitorThread::StorageJanitorThread(QObject* parent): QThread(parent)
{
}

void StorageJanitorThread::run()
{
  StorageJanitor* janitor = new StorageJanitor;
  exec();
  delete janitor;
}

StorageJanitor::StorageJanitor(QObject* parent) :
  QObject(parent),
  m_connection( QDBusConnection::connectToBus( QDBusConnection::SessionBus, QLatin1String(staticMetaObject.className()) ) )
{
  DataStore::self();
  m_connection.registerService( AkDBus::serviceName(AkDBus::StorageJanitor) );
  m_connection.registerObject( QLatin1String( AKONADI_DBUS_STORAGEJANITOR_PATH ), this, QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals );
}

StorageJanitor::~StorageJanitor()
{
  m_connection.unregisterObject( QLatin1String( AKONADI_DBUS_STORAGEJANITOR_PATH ), QDBusConnection::UnregisterTree );
  m_connection.unregisterService( AkDBus::serviceName(AkDBus::StorageJanitor) );
  m_connection.disconnectFromBus( m_connection.name() );

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

  inform( "Verifying external parts..." );
  verifyExternalParts();

  inform( "Looking for dirty objects..." );
  findDirtyObjects();

  /* TODO some ideas for further checks:
   * the collection tree is non-cyclic
   * content type constraints of collections are not violated
   * find unused flags
   * find unused mimetypes
   * check for dead entries in relation tables
   * check if part size matches file size
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
    inform( QLatin1Literal( "Found " ) + QString::number( orphans.size() ) + QLatin1Literal( " orphan parts." ) );
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
  qb.addValueCondition( QLatin1Literal("count(") + Part::idColumn() + QLatin1Literal(")"), Query::Greater, 1, QueryBuilder::HavingCondition );
  qb.exec();

  int count = 0;
  while ( qb.query().next() ) {
    ++count;
    inform( QLatin1Literal( "Found overlapping part data: " ) + qb.query().value( 0 ).toString() );
    // TODO: uh oh, this is bad, how do we recover from that?
  }

  if ( count > 0 )
    inform( QLatin1Literal( "Found " ) + QString::number( count ) + QLatin1Literal( " overlapping parts - bad." ) );
}

void StorageJanitor::verifyExternalParts()
{
  QSet<QString> existingFiles;
  QSet<QString> usedFiles;

  // list all files
  const QString dataDir = AkStandardDirs::saveDir( "data", QLatin1String( "file_db_data" ) );
  QDirIterator it( dataDir );
  while ( it.hasNext() )
    existingFiles.insert( it.next() );
  existingFiles.remove( dataDir + QDir::separator() + QLatin1String( "." ) );
  existingFiles.remove( dataDir + QDir::separator() + QLatin1String( ".." ) );
  inform( QLatin1Literal( "Found " ) + QString::number( existingFiles.size() ) + QLatin1Literal( " external files." ) );

  // list all parts from the db which claim to have an associated file
  QueryBuilder qb( Part::tableName(), QueryBuilder::Select );
  qb.addColumn( Part::dataColumn() );
  qb.addValueCondition( Part::externalColumn(), Query::Equals, true );
  qb.addValueCondition( Part::dataColumn(), Query::IsNot, QVariant() );
  qb.exec();
  while ( qb.query().next() ) {
    const QString partPath = qb.query().value( 0 ).toString();
    if ( existingFiles.contains( partPath ) ) {
      usedFiles.insert( partPath );
    } else {
      inform( QLatin1Literal( "Missing external file: " ) + partPath );
      // TODO: fix this, by clearing the data column?
    }
  }
  inform( QLatin1Literal( "Found " ) + QString::number( usedFiles.size() ) + QLatin1Literal( " external parts." ) );

  // see what's left and move it to lost+found
  const QSet<QString> unreferencedFiles = existingFiles - usedFiles;
  if ( !unreferencedFiles.isEmpty() ) {
    const QString lfDir = AkStandardDirs::saveDir( "data", QLatin1String( "file_lost+found" ) );
    Q_FOREACH ( const QString &file, unreferencedFiles ) {
      inform( QLatin1Literal( "Found unreferenced external file: " ) + file );
      const QFileInfo f( file );
      QFile::rename( file, lfDir + QDir::separator() + f.fileName() );
    }
    inform( QString::fromLatin1("Moved %1 unreferenced files to lost+found.").arg(unreferencedFiles.size()) );
  } else {
    inform( "Found no unreferenced external files." );
  }
}

void StorageJanitor::findDirtyObjects()
{
  SelectQueryBuilder<Collection> cqb;
  cqb.setSubQueryMode( Query::Or);
  cqb.addValueCondition( Collection::remoteIdColumn(), Query::Is, QVariant() );
  cqb.addValueCondition( Collection::remoteIdColumn(), Query::Equals, QString() );
  cqb.exec();
  const Collection::List ridLessCols = cqb.result();
  Q_FOREACH ( const Collection &col, ridLessCols )
    inform( QLatin1Literal( "Collection \"" ) + col.name() + QLatin1Literal( "\" (id: " ) + QString::number( col.id()  )
          + QLatin1Literal( ") has no RID." ) );
  inform( QLatin1Literal( "Found " ) + QString::number( ridLessCols.size() ) + QLatin1Literal( " collections without RID." ) );

  SelectQueryBuilder<PimItem> iqb1;
  iqb1.setSubQueryMode( Query::Or);
  iqb1.addValueCondition( PimItem::remoteIdColumn(), Query::Is, QVariant() );
  iqb1.addValueCondition( PimItem::remoteIdColumn(), Query::Equals, QString() );
  iqb1.exec();
  const PimItem::List ridLessItems = iqb1.result();
  Q_FOREACH ( const PimItem &item, ridLessItems )
    inform( QLatin1Literal( "Item \"" ) + QString::number( item.id()  ) + QLatin1Literal( "\" has no RID." ) );
  inform( QLatin1Literal( "Found " ) + QString::number( ridLessItems.size() ) + QLatin1Literal( " items without RID." ) );

  SelectQueryBuilder<PimItem> iqb2;
  iqb2.addValueCondition( PimItem::dirtyColumn(), Query::Equals, true );
  iqb2.addValueCondition( PimItem::remoteIdColumn(), Akonadi::Query::IsNot, QVariant() );
  iqb2.addSortColumn( PimItem::idFullColumnName() );
  iqb2.exec();
  const PimItem::List dirtyItems = iqb2.result();
  Q_FOREACH ( const PimItem &item, dirtyItems )
    inform( QLatin1Literal( "Item \"" ) + QString::number( item.id()  ) + QLatin1Literal( "\" has RID and is dirty." ) );
  inform( QLatin1Literal( "Found " ) + QString::number( dirtyItems.size() ) + QLatin1Literal( " dirty items." ) );
}

void StorageJanitor::vacuum()
{
  const QString driverName = DataStore::self()->database().driverName();
  if( ( driverName == QLatin1String( "QMYSQL" ) ) || ( driverName == QLatin1String( "QPSQL" ) ) ) {
    inform( "vacuuming database, that'll take some time and require a lot of temporary disk space..." );
    Q_FOREACH ( const QString &table, Akonadi::allDatabaseTables() ) {
      inform( QString::fromLatin1( "optimizing table %1..." ).arg( table ) );

      QString queryStr;
      if ( driverName == QLatin1String( "QMYSQL" ) ) {
        queryStr = QLatin1Literal( "OPTIMIZE TABLE " ) + table;
      } else {
        queryStr = QLatin1Literal( "VACUUM FULL ANALYZE " ) + table;
      }
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
  Q_EMIT information( msg );
}

