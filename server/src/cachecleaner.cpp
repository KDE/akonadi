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
}

void CacheCleaner::run()
{
  DataStore::self();
  QTimer::singleShot( 60 * 1000, this, SLOT(cleanCache()) );
  exec();
  DataStore::self()->close();
}

void CacheCleaner::cleanCache()
{
  qDebug() << "cleaning cache...";

  // cycle over all locations
  QList<Location> locations = Location::retrieveAll();
  foreach ( Location location, locations ) {
    // determine active cache policy
    DataStore::self()->activeCachePolicy( location );

    // check if there is something to expire at all
    if ( location.cachePolicyLocalParts() == QLatin1String( "ALL" ) || location.cachePolicyCacheTimeout() < 0
       || !location.subscribed() )
      continue;
    const int expireTime = qMax( 5, location.cachePolicyCacheTimeout() );

    // find all expired item parts
    SelectQueryBuilder<Part> qb;
    qb.addTable( PimItem::tableName() );
    qb.addColumnCondition( PimItem::idFullColumnName(), Query::Equals, Part::pimItemIdFullColumnName() );
    qb.addValueCondition( PimItem::locationIdFullColumnName(), Query::Equals, location.id() );
    qb.addValueCondition( PimItem::atimeFullColumnName(), Query::Less, QDateTime::currentDateTime().addSecs( -60 * expireTime ) );
    qb.addValueCondition( Part::dataFullColumnName(), Query::IsNot, QVariant() );
    qb.addValueCondition( PimItem::dirtyFullColumnName(), Query::Equals, false );
    if ( !location.cachePolicyLocalParts().isEmpty() )
      qb.addValueCondition( Part::nameFullColumnName(), Query::NotIn, location.cachePolicyLocalParts().split( QLatin1String(" ") ) );
    if ( !qb.exec() )
      continue;
    QList<Part> parts = qb.result();

    if ( parts.isEmpty() )
      continue;
    qDebug() << "found" << parts.count() << "item parts to expire in collection" << location.name();

    // clear data field
    foreach ( Part part, parts ) {
      part.setData( QByteArray() );
      if ( !part.update() )
        qDebug() << "failed to update item part" << part.id();
    }
  }

  qDebug() << "cleaning cache done";
  QTimer::singleShot( 60 * 1000, this, SLOT(cleanCache()) );
}

#include "cachecleaner.moc"
