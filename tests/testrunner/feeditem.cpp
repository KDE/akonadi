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
#include <kurl.h>
#include <akonadi/item.h>
#include <syndication/feed.h>
#include <syndication/loader.h>
#include <QDir>
#include <QtTest>
#include <QEventLoop>


FeedItem::FeedItem(QFile *file, const QString &mimetype)
:Item(mimetype)
{
  KUrl kurl;
  if (!KUrl::isRelativeUrl( file->fileName() ))
    kurl = KUrl( file->fileName() );
  else
    kurl = KUrl("file://" + QDir::currentPath() + '/', file->fileName() );

  Syndication::Loader* loader = Syndication::Loader::create(this,
                                           SLOT(feedLoaded(Syndication::Loader*,
                                           Syndication::FeedPtr,
                                           Syndication::ErrorCode)));
  loader->loadFrom(kurl);

  QEventLoop *test = new QEventLoop(this);
  while(test->processEvents(QEventLoop::WaitForMoreEvents)) {} //workaround
}

void FeedItem::feedLoaded(Syndication::Loader* loader,
                            Syndication::FeedPtr feed,
                            Syndication::ErrorCode error)
{
  Q_UNUSED( loader )
  if( error != Syndication::Success)
    return;

  foreach(const Syndication::ItemPtr &itemptr, feed->items()){
    Akonadi::Item i;
    i.setMimeType(mimetype);
    i.setPayload<Syndication::ItemPtr>( itemptr );
    item.append( i );
  }
}
