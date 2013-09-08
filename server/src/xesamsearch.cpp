/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "xesamsearch.h"

#include "xesaminterface.h"
#include "xesamtypes.h"
#include <akdebug.h>

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

using namespace Akonadi;

static qint64 uriToItemId( const QString &urlString )
{
  const QUrl url( urlString );
  bool ok = false;

  const qint64 id = url.queryItemValue( QLatin1String( "item" ) ).toLongLong( &ok );

  if ( !ok ) {
    return -1;
  } else {
    return id;
  }
}

XesamSearch::XesamSearch( QObject *parent )
  : QObject( parent )
  , mInterface( 0 )
{
  mInterface = new OrgFreedesktopXesamSearchInterface(
      QLatin1String( "org.freedesktop.xesam.searcher" ),
      QLatin1String( "/org/freedesktop/xesam/searcher/main" ),
      QDBusConnection::sessionBus(), this );

  mSession = mInterface->NewSession();

  connect( mInterface, SIGNAL(HitsAdded(QString,uint)), SLOT(hitsAdded(QString,uint)) );
}

XesamSearch::~XesamSearch()
{
  if ( mInterface->isValid() ) {
    mInterface->CloseSession( mSession );
  }
}

QStringList XesamSearch::search( const QString &query )
{
  if ( !mInterface ) {
    qWarning() << "Xesam search service not available!";
    return QStringList();
  }

  QDBusPendingReply<QString> reply = mInterface->NewSearch( mSession, query );
  reply.waitForFinished();
  if ( !reply.isValid() ) {
    akError() << "Xesam search failed: " << reply.error().message();
    return QStringList();
  }
  const QString searchId = reply.value();

  QEventLoop loop;
  connect( mInterface, SIGNAL(SearchDone(QString)), &loop, SLOT(quit()) );
  mInterface->StartSearch( searchId );
  loop.exec();

  mInterface->CloseSearch( searchId );

  return mMatchingUIDs.toList();
}

void XesamSearch::hitsAdded( const QString &search, uint count )
{
  const QVector<QList<QVariant> > results = mInterface->GetHits( search, count );

  typedef QList<QVariant> VariantList;
  Q_FOREACH ( const VariantList &list, results ) {
    if ( list.isEmpty() ) {
      continue;
    }

    const qint64 itemId = uriToItemId( list.first().toString() );
    if ( itemId == -1 ) {
      continue;
    }

    mMatchingUIDs.insert( QString::number( itemId ) );
  }
}
