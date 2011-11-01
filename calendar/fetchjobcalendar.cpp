/*
   Copyright (C) 2011 Sérgio Martins <sergio.martins@kdab.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "fetchjobcalendar.h"
#include "fetchjobcalendar_p.h"
#include "incidencefetchjob.h"

#include <Akonadi/Item>
#include <Akonadi/Collection>

using namespace Akonadi;
using namespace KCalCore;

FetchJobCalendarPrivate::FetchJobCalendarPrivate( FetchJobCalendar *qq ) : AkonadiCalendarPrivate( qq )
                                                                           , q( qq )
{
  IncidenceFetchJob *job = new IncidenceFetchJob();
  connect( job, SIGNAL(result(KJob*)),
           SLOT(slotSearchJobFinished(KJob*)) );
}

FetchJobCalendarPrivate::~FetchJobCalendarPrivate()
{
}

void FetchJobCalendarPrivate::slotSearchJobFinished( KJob *job )
{
  IncidenceFetchJob *searchJob = static_cast<Akonadi::IncidenceFetchJob*>( job );
  if ( searchJob->error() ) {
    //TODO: Error
    kWarning() << "Unable to fetch incidences:" << searchJob->errorText();
  } else {
    foreach( const Akonadi::Item &item, searchJob->items() ) {
      internalInsert( item );
    }
  }
  // emit loaded() in a delayed manner, due to freezes because of execs.
  QMetaObject::invokeMethod( q, "loaded", Qt::QueuedConnection );
}

FetchJobCalendar::FetchJobCalendar( const KDateTime::Spec &timeSpec )
                 : AkonadiCalendar( new FetchJobCalendarPrivate( this ), timeSpec )
{
}

FetchJobCalendar::~FetchJobCalendar()
{
}

#include "fetchjobcalendar.moc"
#include "fetchjobcalendar_p.moc"
