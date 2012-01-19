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

#include "nepomuksearch.h"

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

#include "nepomuk/result.h"

using namespace Akonadi;

static qint64 resultToId( const Nepomuk::Query::Result &result )
{
  const Soprano::Node &property = result.requestProperty(QUrl( QLatin1String( "http://akonadi-project.org/ontologies/aneo#akonadiItemId" ) ));
  if (!(property.isValid() && property.isLiteral() && property.literal().isString())) {
    qWarning() << "Failed to get requested akonadiItemId property";
    qDebug() << "AkonadiItemId missing in query results!" << result.resourceUri() << property.isValid() << property.isLiteral() << property.literal().isString() << property.literal().type() << result.requestProperties().size();
    qDebug() << result.requestProperties().values().first().toString();
    return -1;
  }
  return property.literal().toString().toLongLong();
}

NepomukSearch::NepomukSearch( QObject* parent )
  : QObject( parent ), mSearchService( 0 )
{
  mSearchService = new Nepomuk::Query::QueryServiceClient( this );
  connect( mSearchService, SIGNAL(newEntries(QList<Nepomuk::Query::Result>)),
           this, SLOT(idHitsAdded(QList<Nepomuk::Query::Result>)) );
  mIdSearchService = new Nepomuk::Query::QueryServiceClient( this );
  connect( mSearchService, SIGNAL(newEntries(QList<Nepomuk::Query::Result>)),
           this, SLOT(hitsAdded(QList<Nepomuk::Query::Result>)) );
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

void NepomukSearch::hitsAdded( const QList<Nepomuk::Query::Result>& entries )
{
  if ( !mSearchService ) {
    qWarning() << "Nepomuk search service not available!";
    return;
  }

  Q_FOREACH( const Nepomuk::Query::Result &result, entries ) {
    QHash<QString, QString> encodedRps;
    //We do another query to get the akonadiItemId attribute (since it may not have been added to the original query)
    encodedRps.insert( QString::fromLatin1( "reqProp1" ), QUrl(QString::fromLatin1("http://akonadi-project.org/ontologies/aneo#akonadiItemId")).toString() );
    mIdSearchService->blockingQuery(QString::fromLatin1("SELECT DISTINCT ?r ?reqProp1 WHERE { %1 a ?v2 . %1 <http://akonadi-project.org/ontologies/aneo#akonadiItemId> ?reqProp1 . } LIMIT 1").arg(result.resourceUri().toString()), encodedRps);
  }
}

void NepomukSearch::idHitsAdded(const QList< Nepomuk::Query::Result >& entries)
{

  Q_FOREACH( const Nepomuk::Query::Result &result, entries ) {
    const qint64 itemId = resultToId( result );

    if ( itemId == -1 )
      continue;

    mMatchingUIDs.insert( QString::number( itemId ) );
  }

}


#include "nepomuksearch.moc"
