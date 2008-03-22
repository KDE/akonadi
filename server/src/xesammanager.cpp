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

#include "xesammanager.h"

#include "storage/datastore.h"
#include "xesaminterface.h"
#include "xesamtypes.h"

#include <QDebug>

using namespace Akonadi;

XesamManager* XesamManager::mInstance = 0;

XesamManager::XesamManager(QObject * parent) :
    QObject( parent ),
    mValid( true )
{
  Q_ASSERT( mInstance == 0 );
  mInstance = this;

  mInterface = new OrgFreedesktopXesamSearchInterface(
      QLatin1String("org.freedesktop.xesam.searcher"),
      QLatin1String("/org/freedesktop/xesam/searcher/main"),
      QDBusConnection::sessionBus(), this );

  if ( mInterface->isValid() ) {
    mSession = mInterface->NewSession();
    QDBusVariant result = mInterface->SetProperty( mSession, QLatin1String("search.live"), QDBusVariant(true) );
    mValid = mValid && result.variant().toBool();
    result = mInterface->SetProperty( mSession, QLatin1String("search.blocking"), QDBusVariant(false) );
    mValid = mValid && !result.variant().toBool();
    qDebug() << "XESAM session:" << mSession;

    connect( mInterface, SIGNAL(HitsAdded(QString,int)), SLOT(slotHitsAdded(QString,int)) );
    connect( mInterface, SIGNAL(HitsRemoved(QString,QList<int>)), SLOT(slotHitsRemoved(QString,QList<int>)) );
    connect( mInterface, SIGNAL(HitsModified(QString,QList<int>)), SLOT(slotHitsModified(QString,QList<int>)) );

    reloadSearches();
  } else {
    qWarning() << "XESAM interface not found!";
    mValid = false;
  }

  if ( !mValid )
    qWarning() << "No valid XESAM interface found!";
}

XesamManager::~XesamManager()
{
  stopSearches();
  if ( !mSession.isEmpty() )
    mInterface->CloseSession( mSession );
  mInstance = 0;
}

void XesamManager::slotHitsAdded(const QString & search, int count)
{
  qDebug() << "hits added: " << search << count;
  mMutex.lock();
  qint64 colId = mSearchMap.value( search );
  mMutex.unlock();
  if ( colId <= 0 || count <= 0 )
    return;
  qDebug() << "calling GetHits";
  QList<QList<QVariant> > results = mInterface->GetHits( search, count );
  qDebug() << "GetHits returned:" << results.count();
  typedef QList<QVariant> VariantList;
  foreach ( const VariantList list, results ) {
    if ( list.isEmpty() )
      continue;
    qint64 itemId = uriToItemId( list.first().toString() );
    Entity::addToRelation<LocationPimItemRelation>( colId, itemId );
  }
}

void XesamManager::slotHitsRemoved(const QString & search, const QList<int> & hits)
{
  qDebug() << "hits removed: " << search << hits;
  mMutex.lock();
  qint64 colId = mSearchMap.value( search );
  mMutex.unlock();
  if ( colId <= 0 )
    return;
  QList<QList<QVariant> > results = mInterface->GetHitData( search, hits, QStringList( QLatin1String("uri") ) );
  typedef QList<QVariant> VariantList;
  foreach ( const VariantList list, results ) {
    if ( list.isEmpty() )
      continue;
    qint64 itemId = uriToItemId( list.first().toString() );
    Entity::removeFromRelation<LocationPimItemRelation>( colId, itemId );
  }
}

void XesamManager::slotHitsModified(const QString & search, const QList< int > & hits)
{
  qDebug() << "hits modified: " << search << hits;
}

void XesamManager::reloadSearches()
{
  Resource res = Resource::retrieveByName( QLatin1String("akonadi_search_resource") );
  if ( !res.isValid() ) {
    qWarning() << "No valid search resource found!";
    return;
  }
  Location::List locs = res.locations();
  foreach ( const Location l, locs ) {
    addSearch( l );
  }
}

bool XesamManager::addSearch(const Location & loc)
{
  if ( !mInterface->isValid() || !mValid )
    return false;
  QMutexLocker lock( &mMutex );
  if ( loc.remoteId().isEmpty() )
    return false;
  QString searchId = mInterface->NewSearch( mSession, loc.remoteId() );
  qDebug() << "XesamManager::addSeach" << loc << searchId;
  mSearchMap[ searchId ] = loc.id();
  mInvSearchMap[ loc.id() ] = searchId;
  mInterface->StartSearch( searchId );

#if 0
  // ### workaround until HitAdded is emitted by strigi
  lock.unlock();
  int count = mInterface->CountHits( searchId );
  slotHitsAdded( searchId, count );
#endif

  return true;
}

bool XesamManager::removeSearch(qint64 loc)
{
  QMutexLocker lock( &mMutex );
  if ( !mInvSearchMap.contains( loc ) )
    return false;
  QString searchId = mInvSearchMap.value( loc );
  mInvSearchMap.remove( loc );
  mSearchMap.remove( searchId );
  return true;
}

void XesamManager::stopSearches()
{
  Resource res = Resource::retrieveByName( QLatin1String("akonadi_search_resource") );
  if ( !res.isValid() ) {
    qWarning() << "No valid search resource found!";
    return;
  }
  Location::List locs = res.locations();
  foreach ( const Location l, locs ) {
    removeSearch( l.id() );
  }
}

qint64 XesamManager::uriToItemId(const QString & uri)
{
  // TODO implement me!
  return uri.toLongLong();
}

#include "xesammanager.moc"
