/*
   Copyright (C) 2011 Sérgio Martins <sergio.martins@kdab.com>

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
