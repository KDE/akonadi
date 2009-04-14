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
#include "storage/parthelper.h"
#include "storage/datastore.h"
#include "storage/selectquerybuilder.h"

#include <QDebug>
#include <QTimer>

using namespace Akonadi;

CacheCleaner::CacheCleaner(QObject * parent) :
    QThread( parent )
{
  mTime = 60;
  mLoops = 0;
}

CacheCleaner::~CacheCleaner()
{
}

void CacheCleaner::run()
{
  DataStore::self();
  QTimer::singleShot( mTime * 1000, this, SLOT(cleanCache()) );
  exec();
  DataStore::self()->close();
}

void CacheCleaner::cleanCache()
{
  qint64 loopsWithExpiredItem = 0;
  // cycle over all collection
  QList<Collection> collections = Collection::retrieveAll();
  foreach ( Collection collection, collections ) {
    // determine active cache policy
    DataStore::self()->activeCachePolicy( collection );

    // check if there is something to expire at all
    if ( collection.cachePolicyLocalParts() == QLatin1String( "ALL" ) || collection.cachePolicyCacheTimeout() < 0
       || !collection.subscribed() || !collection.resourceId() )
      continue;
    const int expireTime = qMax( 5, collection.cachePolicyCacheTimeout() );

    // find all expired item parts
    SelectQueryBuilder<Part> qb;
    qb.addTable( PimItem::tableName() );
    qb.addColumnCondition( PimItem::idFullColumnName(), Query::Equals, Part::pimItemIdFullColumnName() );
    qb.addValueCondition( PimItem::collectionIdFullColumnName(), Query::Equals, collection.id() );
    qb.addValueCondition( PimItem::atimeFullColumnName(), Query::Less, QDateTime::currentDateTime().addSecs( -60 * expireTime ) );
    qb.addValueCondition( Part::dataFullColumnName(), Query::IsNot, QVariant() );
    qb.addValueCondition( QString::fromLatin1( "substr( %1, 1, 4 )" ).arg( Part::nameFullColumnName() ), Query::Equals, QLatin1String( "PLD:" ) );
    qb.addValueCondition( PimItem::dirtyFullColumnName(), Query::Equals, false );
    if ( !collection.cachePolicyLocalParts().isEmpty() )
      qb.addValueCondition( Part::nameFullColumnName(), Query::NotIn, collection.cachePolicyLocalParts().split( QLatin1String(" ") ) );
    if ( !qb.exec() )
      continue;
    Part::List parts = qb.result();
    PartHelper::loadData(parts); //FIXME: not needed anymore to read back the data itself?

    if ( parts.isEmpty() )
      continue;
    qDebug() << "found" << parts.count() << "item parts to expire in collection" << collection.name();

    // clear data field
    for ( int i = 0; i < parts.count(); ++i) {
      if ( !PartHelper::update( &(parts[ i ]), QByteArray(), 0) )
        qDebug() << "failed to update item part" << parts[ i ].id();
    }
    loopsWithExpiredItem++;
  }

  /* if we have item parts to expire in collection the mTime is
   * decreased of 60 and if there are lot of collection need to be clean
   * mTime is 60 otherwise we increment mTime in 60
   */

  if( mLoops < loopsWithExpiredItem) {
    if( (mTime > 60) && (loopsWithExpiredItem - mLoops) < 50 )
      mTime -= 60;
    else
      mTime = 60;
  } else {
    if( mTime < 600)
      mTime += 60;
  }

  // measured arithmetic between mLoops and loopsWithExpiredItem
  mLoops = (loopsWithExpiredItem + mLoops) >> 2;

  QTimer::singleShot( mTime * 1000, this, SLOT(cleanCache()) );
}

#include "cachecleaner.moc"
