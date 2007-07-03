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

#include "xesaminterface.h"
#include "xesamtypes.h"

#include <QDebug>

using namespace Akonadi;

XesamManager::XesamManager(QObject * parent) :
    QObject( parent )
{
  mInterface = new OrgFreedesktopXesamSearchInterface(
      QLatin1String("org.freedesktop.xesam.searcher"),
      QLatin1String("/org/freedesktop/xesam/searcher/main"),
      QDBusConnection::sessionBus(), this );

  if ( mInterface->isValid() ) {
    mSession = mInterface->NewSession();
    QDBusVariant result = mInterface->SetProperty( mSession, QLatin1String("search.live"), QDBusVariant(true) );
    Q_ASSERT( result.variant().toBool() );
    result = mInterface->SetProperty( mSession, QLatin1String("search.blocking"), QDBusVariant(false) );
    Q_ASSERT( !result.variant().toBool() );
    qDebug() << "XESAM session:" << mSession;

    connect( mInterface, SIGNAL(HitsAdded(QString,int)), SLOT(slotHitsAdded(QString,int)) );
    connect( mInterface, SIGNAL(HitsRemoved(QString,QList<int>)), SLOT(slotHitsRemoved(QString,QList<int>)) );
    connect( mInterface, SIGNAL(HitsModified(QString,QList<int>)), SLOT(slotHitsModified(QString,QList<int>)) );

    // testing
    mSearch = mInterface->NewSearch( mSession, QLatin1String("search-query") );
    mInterface->StartSearch( mSearch );
  } else {
    qWarning() << "XESAM interface not found!";
  }
}

XesamManager::~XesamManager()
{
  if ( !mSession.isEmpty() )
    mInterface->CloseSession( mSession );
}

void XesamManager::slotHitsAdded(const QString & search, int count)
{
  qDebug() << "hits added: " << search << count;
}

void XesamManager::slotHitsRemoved(const QString & search, const QList<int> & hits)
{
  qDebug() << "hits removed: " << search << hits;
}

void XesamManager::slotHitsModified(const QString & search, const QList< int > & hits)
{
  qDebug() << "hits modified: " << search << hits;
}

#include "xesammanager.moc"
