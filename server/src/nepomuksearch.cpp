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

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

#include "nepomuksearch.h"

#include "search/result.h"

using namespace Akonadi;

static qint64 uriToItemId( const QUrl &url )
{
  bool ok = false;

  const qint64 id = url.queryItemValue( QLatin1String( "item" ) ).toLongLong( &ok );

  if ( !ok )
    return -1;
  else
    return id;
}

NepomukSearch::NepomukSearch( QObject* parent )
  : QObject( parent ), mSearchService( 0 )
{
  mSearchService = new Nepomuk::Search::QueryServiceClient( this );
  connect( mSearchService, SIGNAL( newEntries( const QList<Nepomuk::Search::Result>& ) ),
           this, SLOT( hitsAdded( const QList<Nepomuk::Search::Result>& ) ) );
}

NepomukSearch::~NepomukSearch()
{
  if ( mSearchService ) {
    mSearchService->close();
    delete mSearchService;
  }
}

QStringList NepomukSearch::search( const QString &query )
{
  if ( !mSearchService ) {
    qWarning() << "Nepomuk search service not available!";
    return QStringList();
  }

  if ( !mSearchService->blockingQuery( query ) ) {
    qWarning() << Q_FUNC_INFO << "Calling blockingQuery() failed!";
    return QStringList();
  }

  return mMatchingUIDs.toList();
}

void NepomukSearch::hitsAdded( const QList<Nepomuk::Search::Result>& entries )
{
  if ( !mSearchService ) {
    qWarning() << "Nepomuk search service not available!";
    return;
  }

  Q_FOREACH( const Nepomuk::Search::Result &result, entries ) {
    const qint64 itemId = uriToItemId( result.resourceUri() );

    if ( itemId == -1 )
      continue;

    mMatchingUIDs.insert( QString::number( itemId ) );
  }
}

#include "nepomuksearch.moc"
