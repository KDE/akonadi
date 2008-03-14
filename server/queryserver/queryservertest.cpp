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

#include <QtCore/QCoreApplication>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>

#include "searchqueryiteratorinterface.h"

#include "queryservertest.h"

TestObject::TestObject( const QString &query, QObject *parent )
  : QObject( parent )
{
  mSearch = new org::kde::Akonadi::Search( "org.kde.Akonadi.Search", "/Search", QDBusConnection::sessionBus(), this );

  const QString queryPath = mSearch->createQuery( query );

  mQuery = new org::kde::Akonadi::SearchQuery( "org.kde.Akonadi.Search", queryPath, QDBusConnection::sessionBus(), this );

  const QString iteratorPath = mQuery->allHits();

  org::kde::Akonadi::SearchQueryIterator *iterator = new org::kde::Akonadi::SearchQueryIterator( "org.kde.Akonadi.Search", iteratorPath,
                                                                         QDBusConnection::sessionBus(), this );

  while ( iterator->next() ) {
    QPair<QString, double> data = iterator->current();
    qDebug("  %s", qPrintable( data.first ) );
  }

  iterator->close();

  connect( mQuery, SIGNAL( hitsChanged( const QMap<QString, double>& ) ),
           this, SLOT( hitsChanged( const QMap<QString, double>& ) ) );
  connect( mQuery, SIGNAL( hitsChanged( const QMap<QString, double>& ) ),
           this, SLOT( hitsRemoved( const QMap<QString, double>& ) ) );

  mQuery->start();
}

void TestObject::hitsChanged( const QMap<QString, double> &hits )
{
  qDebug( "New hits:" );
  QMapIterator<QString, double> it( hits );
  while ( it.hasNext() ) {
    it.next();
    qDebug( "  %s: %f", qPrintable( it.key() ), it.value() );
  }
}

void TestObject::hitsRemoved( const QMap<QString, double> &hits )
{
  qDebug( "Removed hits:" );
  QMapIterator<QString, double> it( hits );
  while ( it.hasNext() ) {
    it.next();
    qDebug( "  %s: %f", qPrintable( it.key() ), it.value() );
  }
}

int main( int argc, char **argv )
{
  QCoreApplication app( argc, argv );

  if ( argc != 2 ) {
    qDebug( "usage: queryservertest query" );
    return 1;
  }

  new TestObject( QString::fromLatin1( argv[ 1 ] ) );

  return app.exec();
}

#include "queryservertest.moc"
