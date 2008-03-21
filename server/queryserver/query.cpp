/***************************************************************************
 *   Copyright (C) 2008 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QtCore/QDateTime>
#include <QtCore/QSet>

#include <Soprano/Node>

#include "searchqueryadaptor.h"
#include "queryiterator.h"

#include "query.h"

class Query::Private
{
  public:
    Private( Query *parent, const QString &queryString, Soprano::Model *model, const QString &id )
      : mParent( parent ), mId( id ), mIteratorIdCounter( 0 ),
        mQueryString( queryString ), mModel( model ), mRunning( false )
    {
    }

    QString createUniqueIteratorId()
    {
      return QString( "%1/Iterator%2" ).arg( mId ).arg( ++mIteratorIdCounter );
    }

    void _k_statementsAdded();
    void _k_statementsRemoved();

    Query *mParent;
    QString mId;
    unsigned int mIteratorIdCounter;
    QString mQueryString;
    QDateTime mQueryTime;
    Soprano::Model *mModel;
    QSet<QueryIterator*> mIterators;
    QSet<QString> mAllUris;
    bool mRunning;
};

void Query::Private::_k_statementsAdded()
{
  QList<QString> uris;
  Soprano::QueryResultIterator it = mModel->executeQuery( mQueryString, Soprano::Query::QueryLanguageSparql );

  /**
   * Check for uris which exist in the model but not
   * in our cache -> they are new
   */
  while ( it.next() ) {
    const QString uri = it.binding( 0 ).uri().toString();

    if ( !mAllUris.contains( uri ) ) {
      mAllUris.insert( uri );
      uris.append( uri );
    }
  }

  mQueryTime = QDateTime::currentDateTime();

  emit mParent->hitsChanged( uris );
}

void Query::Private::_k_statementsRemoved()
{
  QSet<QString> availableUris;

  Soprano::QueryResultIterator it = mModel->executeQuery( mQueryString, Soprano::Query::QueryLanguageSparql );
  while ( it.next() ) {
    const QString uri = it.binding( 0 ).uri().toString();
    availableUris.insert( uri );
  }

  /**
   * Check for uris which exist in our cache but not in
   * in the model -> they were deleted.
   */
  QSet<QString> tmp = mAllUris;
  tmp.subtract( availableUris );  // tmp contains all deleted uris

  /**
   * Remove all deleted uris from our cache.
   */
  mAllUris.subtract( tmp );

  /**
   * Create result map.
   */
  QList<QString> uris;
  Q_FOREACH( const QString uri, tmp )
    uris.append( uri );

  mQueryTime = QDateTime::currentDateTime();

  emit mParent->hitsRemoved( uris );
}

Query::Query( const QString &queryString, Soprano::Model *model, const QString &id, QObject *parent )
  : QObject( parent ), d( new Private( this, queryString, model, id ) )
{
  new SearchQueryAdaptor( this );

  QDBusConnection::sessionBus().registerObject( id, this );
}

Query::~Query()
{
  delete d;
}

void Query::start()
{
  if ( d->mRunning )
    return;

  d->mRunning = true;

  /**
   * Stores all available items, so we can find out which uris have been added/deleted.
   *
   * TODO: change it when Nepomuk will support search by creation time.
   */
  Soprano::QueryResultIterator it = d->mModel->executeQuery( d->mQueryString, Soprano::Query::QueryLanguageSparql );
  while ( it.next() )
    d->mAllUris.insert( it.binding( 0 ).uri().toString() );

  it.close();

  connect( d->mModel, SIGNAL( statementsAdded() ), this, SLOT( _k_statementsAdded() ) );
  connect( d->mModel, SIGNAL( statementsRemoved() ), this, SLOT( _k_statementsRemoved() ) );
}

void Query::stop()
{
  if ( !d->mRunning )
    return;

  d->mRunning = false;

  disconnect( d->mModel, SIGNAL( statementsAdded() ), this, SLOT( _k_statementsAdded() ) );
  disconnect( d->mModel, SIGNAL( statementsRemoved() ), this, SLOT( _k_statementsRemoved() ) );
}

void Query::close()
{
  deleteLater();
}

QString Query::allHits()
{
  const QString id = d->createUniqueIteratorId();

  Soprano::QueryResultIterator it = d->mModel->executeQuery( d->mQueryString, Soprano::Query::QueryLanguageSparql );

  QueryIterator *iterator = new QueryIterator( it, id, this );
  d->mIterators.insert( iterator );

  return id;
}

#include "query.moc"
