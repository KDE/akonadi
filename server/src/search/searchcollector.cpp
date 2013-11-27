/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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
#include "searchcollector.h"
#include "akdbus.h"
#include "akdebug.h"

#include "agentsearchinterface.h"
#include "resourceinterface.h"

using namespace Akonadi;

static SearchCollector *SearchCollector::m_instance = 0;

SearchCollector* SearchCollector::self()
{
  if ( m_instance == 0 ) {
    m_instance = new SearchCollector;
  }

  return m_instance;
}


SearchCollector::SearchCollector()
  : mDBusConnection(
      QDBusConnection::connectToBus(
          QDBusConnection::SessionBus,
          QString::fromLatin1( "AkonadiServerSearchCollector" ) ) )
{
  connect( mDBusConnection.interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
           this, SLOT(serviceOwnerChanged(QString,QString,QString)) );
}

void SearchCollector::serviceOwnerChanged( const QString &serviceName,
                                           const QString &oldOwner,
                                           const QString &newOwner )
{
  Q_UNUSED( newOwner );
  if ( oldOwner.isEmpty() ) {
    return;
  }

  AkDBus::AgentType type = AkDBus::Unknown;
  const QString resourceId = AkDBus::parseAgentServiceName( serviceName, type );
  if ( resourceId.isEmpty() || type != AkDBus::Resource ) {
    return;
  }

  mResourceInterfaces.remove( resourceId );
  mSearchInterfaces.remove( resourceId );
}

SearchCollector::~SearchCollector()
{
}

QVector<qint64> SearchCollector::exec( SearchTask *task, bool *ok )
{
  Q_FOREACH ( const QByteArray &resource, task->collections.uniqueKeys() ) {
    OrgFreedesktopAkonadiAgentSearchInterface *iface = findInterface( resource, mSearchInterfaces );

    // Does not support search capability
    if ( !iface || !iface->isValid() ) {
      continue;
    }
  }
}

void SearchCollector::pushResults( const QByteArray &searchId, const QSet<qint64> &results )
{

}
