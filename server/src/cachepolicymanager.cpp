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

#include "cachepolicymanager.h"
#include "storage/datastore.h"

#include <QDBusConnection>

using namespace Akonadi;

CachePolicyManager::CachePolicyManager(QObject * parent) :
    QObject( parent )
{
  QDBusConnection::sessionBus().registerObject( QLatin1String("/CachePolicyManager"),
    this, QDBusConnection::ExportScriptableContents );
}

QStringList CachePolicyManager::cachePolicies()
{
  QList<CachePolicy> policies = CachePolicy::retrieveAll();
  QStringList list;
  foreach ( CachePolicy p, policies ) {
    list << QString::fromLatin1( "%1,%2,%3,%4" ).arg( p.id() ).arg( p.name() )
        .arg( p.expireTime() ).arg( p.offlineParts() );
  }
  return list;
}

bool CachePolicyManager::deleteCachePolicy(int id)
{
  return CachePolicy::remove( id );
}

int CachePolicyManager::addCachePolicy(const QString & name, int expireTime, const QString & offlineParts)
{
  if ( CachePolicy::exists( name ) ) {
    qDebug() << "A CachePolicy with this name already exists.";
    return -1;
  }
  int id = -1;
  CachePolicy policy( name, expireTime, offlineParts );
  if ( !policy.insert( &id ) )
    return -1;
  return id;
}

#include "cachepolicymanager.moc"
