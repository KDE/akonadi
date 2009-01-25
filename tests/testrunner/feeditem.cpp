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

#include "feeditem.h"

#include <akonadi/item.h>
#include <kurl.h>
#include <syndication/feed.h>
#include <syndication/loader.h>

#include <QtCore/QDir>
#include <QtCore/QEventLoop>


FeedItem::FeedItem( const QString &fileName, const QString &mimetype )
  : Item( mimetype )
{
  KUrl url;
  if ( !KUrl::isRelativeUrl( fileName ) )
    url = KUrl( fileName );
  else
    url = KUrl( QLatin1String( "file://" ) + QDir::currentPath() + QLatin1Char( '/' ), fileName );

  Syndication::Loader* loader = Syndication::Loader::create( this,
                                           SLOT( feedLoaded( Syndication::Loader*,
                                                             Syndication::FeedPtr,
                                                             Syndication::ErrorCode ) ) );
  loader->loadFrom( url );

  QEventLoop *test = new QEventLoop( this );
  while( test->processEvents( QEventLoop::WaitForMoreEvents ) ) {} //workaround
}

void FeedItem::feedLoaded( Syndication::Loader* loader,
                           Syndication::FeedPtr feed,
                           Syndication::ErrorCode error )
{
  Q_UNUSED( loader )
  if ( error != Syndication::Success )
    return;

  foreach ( const Syndication::ItemPtr &itemptr, feed->items() ) {
    Akonadi::Item item;
    item.setMimeType( mMimeType );
    item.setPayload<Syndication::ItemPtr>( itemptr );
    mItems.append( item );
  }
}
