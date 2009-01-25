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

#include "itemfactory.h"

#include "calitem.h"
#include "feeditem.h"
#include "vcarditem.h"

Item *ItemFactory::createItem( const QString &fileName )
{
  if ( fileName.endsWith( QLatin1String( ".vcf" ) ) ) {
    return new VCardItem( fileName, QLatin1String( "text/vcard" ) );
  } else if ( fileName.endsWith( QLatin1String( ".ics" ) ) ) {
    return new CalItem( fileName, QLatin1String( "text/calendar" ) );
  } else if ( fileName.endsWith( QLatin1String( ".xml" ) ) ) {
    return new FeedItem( fileName, QLatin1String( "application/rss+xml" ) );
  }

  return 0;
}
