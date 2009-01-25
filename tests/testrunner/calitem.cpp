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

#include <akonadi/item.h>
#include <boost/shared_ptr.hpp>
#include <kcal/calendarlocal.h>
#include <kcal/incidence.h>

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

CalItem::CalItem( const QString &fileName, const QString &mimeType )
  : Item( mimeType )
{
  KCal::CalendarLocal calendarlocal( KDateTime::UTC );

  if ( calendarlocal.load( fileName ) ) {
    KCal::Incidence::List incidence = calendarlocal.rawIncidences();
    for ( int i = 0; i < incidence.size(); i++ ) {
      Akonadi::Item item;
      item.setMimeType( mMimeType );
      item.setPayload<IncidencePtr>( IncidencePtr( incidence.at( i )->clone() ) );
      mItems.append( item );
    }
  }
}
