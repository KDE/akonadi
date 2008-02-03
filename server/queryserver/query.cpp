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

#include <QtCore/QSet>

#include <Soprano/Node>

#include "queryadaptor.h"
#include "queryiterator.h"

#include "query.h"

class Query::Private
{
  public:
    Private( Query *parent, const QString &queryString, Soprano::Model *model, const QString &id )
      : mId( id ), mIteratorIdCounter( 0 ), mQueryString( queryString ), mModel( model )
    {
      connect( model, SIGNAL( statementsAdded() ), parent, SLOT( statementsAdded() ) );
      connect( model, SIGNAL( statementsRemoved() ), parent, SLOT( statementsRemoved() ) );
    }

    QString createUniqueIteratorId()
    {
      return QString( "%1/Iterator%2" ).arg( mId ).arg( ++mIteratorIdCounter );
    }

    void _k_statementsAdded();
    void _k_statementsRemoved();

    QString mId;
    unsigned int mIteratorIdCounter;
    QString mQueryString;
    Soprano::Model *mModel;
    QSet<QueryIterator*> mIterators;
};

void Query::Private::_k_statementsAdded()
{

}

void Query::Private::_k_statementsRemoved()
{

}

Query::Query( const QString &queryString, Soprano::Model *model, const QString &id, QObject *parent )
  : QObject( parent ), d( new Private( this, queryString, model, id ) )
{
  new QueryAdaptor( this );

  QDBusConnection::sessionBus().registerObject( id, this );
}

Query::~Query()
{
  delete d;
}

void Query::start()
{
}

void Query::stop()
{
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
