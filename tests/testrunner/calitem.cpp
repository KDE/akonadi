/*
 * Copyright (c) 2008  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "calitem.h"
#include "item.h"
#include <kcal/calendarlocal.h>
#include <akonadi/item.h>
#include <kcal/incidence.h>
#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

CalItem::CalItem( QFile *file, const QString &mimetype)
:Item(mimetype) {
  KCal::CalendarLocal calendarlocal(KDateTime::UTC);
  
  if( calendarlocal.load(file->fileName() )){
    KCal::Incidence::List incidence = calendarlocal.rawIncidences();
    for(int i = 0; i < incidence.size(); i++){
      Akonadi::Item itemtmp;
      itemtmp.setMimeType(mimetype);
      itemtmp.setPayload<IncidencePtr>(IncidencePtr( incidence.at(i)->clone() ));
      item.append(itemtmp);
    }
  }
}

