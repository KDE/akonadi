/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "intervalcheck.h"
#include "storage/datastore.h"

#include <QDebug>
#include <QTimer>

using namespace Akonadi;

IntervalCheck::IntervalCheck(QObject * parent) :
    QThread( parent )
{
}

IntervalCheck::~ IntervalCheck()
{
  DataStore::self()->close();
}

void IntervalCheck::run()
{
  QTimer::singleShot( 60 * 1000, this, SLOT(doIntervalCheck()) );
  exec();
}

void IntervalCheck::doIntervalCheck()
{
  qDebug() << "doing interval checks...";

  // cycle over all locations
  QList<Location> locations = Location::retrieveAll();
  foreach ( Location location, locations ) {
    // determine active cache policy
    DataStore::self()->activeCachePolicy( location );

    // check if there is something to expire at all
    if ( location.cachePolicyCheckInterval() < 0 )
      continue;

    QDateTime lastExpectedCheck = QDateTime::currentDateTime().addSecs( location.cachePolicyCheckInterval() * -60 );
    if ( mLastChecks.contains( location.id() ) && mLastChecks.value( location.id() ) > lastExpectedCheck )
      continue;

    mLastChecks[ location.id() ] = QDateTime::currentDateTime();
    qDebug() << "interval checking  collection " << location.id() << "(" << location.name() << ")";
    DataStore::self()->triggerCollectionSync( location );
  }

  qDebug() << "interval checks done";

  QTimer::singleShot( 60 * 1000, this, SLOT(doIntervalCheck()) );
}

#include "intervalcheck.moc"
