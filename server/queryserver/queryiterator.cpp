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

#include "queryiterator.h"

#include "queryiteratoradaptor.h"

#include <Soprano/Node>

class QueryIterator::Private
{
  public:
    Private( const Soprano::QueryResultIterator &it )
      : mIterator( it )
    {
    }

    Soprano::QueryResultIterator mIterator;
};

QueryIterator::QueryIterator( const Soprano::QueryResultIterator &it, const QString &id, QObject *parent )
  : QObject( parent ), d( new Private( it ) )
{
  new QueryIteratorAdaptor( this );

  QDBusConnection::sessionBus().registerObject( id, this );
}

QueryIterator::~QueryIterator()
{
  delete d;
}

bool QueryIterator::next()
{
  return d->mIterator.next();
}

QPair<QString, double> QueryIterator::current()
{
  return qMakePair( d->mIterator.binding( "uri" ).uri().toString(), (double)0 );
}

void QueryIterator::close()
{
  deleteLater();
}

#include "queryiterator.moc"
