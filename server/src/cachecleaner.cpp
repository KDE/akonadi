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

#include "cachecleaner.h"
#include "storage/datastore.h"
#include "storage/selectquerybuilder.h"

#include <QDebug>
#include <QTimer>

using namespace Akonadi;

CacheCleaner::CacheCleaner(QObject * parent) :
    QThread( parent )
{
}

CacheCleaner::~CacheCleaner()
{
  DataStore::self()->close();
}

void CacheCleaner::run()
{
  QTimer::singleShot( /*60, just for testing --> */ 5 * 1000, this, SLOT(cleanCache()) );
  exec();
}

void CacheCleaner::cleanCache()
{
  qDebug() << "cleaning cache...";

  // cycle over all locations
  QList<Location> locations = Location::retrieveAll();
  foreach ( const Location location, locations ) {
    // determine active cache policy
    CachePolicy policy = DataStore::self()->activeCachePolicy( location );
    if ( !policy.isValid() ) {
      qDebug() << "No valid cache policy found for localtion" << location.name();
      continue;
    }

    // check if there is something to expire at all
    if ( policy.offlineParts() == QLatin1String( "ALL" ) || policy.expireTime() < 0 )
      continue;
    const int expireTime = qMax( 5, policy.expireTime() );

    // find all expired items
    SelectQueryBuilder<PimItem> qb;
    qb.addValueCondition( PimItem::locationIdColumn(), "=", location.id() );
    qb.addValueCondition( PimItem::atimeColumn(), "<", QDateTime::currentDateTime().addSecs( -60 * expireTime ) );
    qb.addValueCondition( PimItem::dataColumn(), "IS NOT", QVariant() );
    qb.addValueCondition( PimItem::dirtyColumn(), "=", false );
    if ( !qb.exec() )
      continue;
    QList<PimItem> pimItems = qb.result();

    if ( pimItems.isEmpty() )
      continue;
    qDebug() << "found" << pimItems.count() << "items to expire in collection" << location.name();

    // clear data field
    foreach ( PimItem pimItem, pimItems ) {
      pimItem.setData( QByteArray() );
      if ( !pimItem.update() )
        qDebug() << "failed to update item" << pimItem.id();
    }
  }

  qDebug() << "cleaning cache done";
  QTimer::singleShot( 60 * 1000, this, SLOT(cleanCache()) );
}

#include "cachecleaner.moc"
