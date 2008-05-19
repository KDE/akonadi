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

#include <QtDBus/QDBusConnection>
#include <QtCore/QSet>

#include <Soprano/Client/DBusClient>
#include <Soprano/Client/DBusModel>

#include "query.h"
#include "searchadaptor.h"

#include "search.h"

class Search::Private
{
  public:
    Private()
      : mQueryIdCounter( 0 )
    {
    }

    QString createUniqueQueryId()
    {
      return QString( "/Search/Query%1" ).arg( ++mQueryIdCounter );
    }

    unsigned int mQueryIdCounter;
    QSet<Query*> mQueries;
    Soprano::Model *mModel;
    Soprano::Client::DBusClient *mSopranoClient;
};

Search::Search( QObject *parent )
  : QObject( parent ), d( new Private )
{
  new SearchAdaptor( this );

  QDBusConnection::sessionBus().registerObject( "/Search", this );

  d->mSopranoClient = new Soprano::Client::DBusClient( "org.kde.NepomukServer", this );
  d->mModel = d->mSopranoClient->createModel( "main" );
}

Search::~Search()
{
  delete d;
}

QString Search::createQuery( const QString &queryString )
{
  const QString id = d->createUniqueQueryId();

  Query *query = new Query( queryString, d->mModel, id, this );
  d->mQueries.insert( query );

  return id;
}

#include "search.moc"
