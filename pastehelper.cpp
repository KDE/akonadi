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

#include "pastehelper.h"

#include "collectioncopyjob.h"
#include "collectionmodifyjob.h"
#include "item.h"
#include "itemappendjob.h"
#include "itemcopyjob.h"
#include "itemstorejob.h"

#include <KDebug>
#include <KUrl>

#include <QByteArray>
#include <QMimeData>

using namespace Akonadi;

bool PasteHelper::canPaste(const QMimeData * mimeData, const Collection & collection)
{
  if ( !mimeData || !collection.isValid() )
    return false;

  if ( KUrl::List::canDecode( mimeData ) )
    return true;

  foreach ( const QString format, mimeData->formats() )
    if ( collection.contentTypes().contains( format ) )
      return true;

  return false;
}

QList<KJob*> PasteHelper::paste(const QMimeData * mimeData, const Collection & collection, bool copy)
{
  QList<KJob*> rv;
  if ( !canPaste( mimeData, collection ) )
    return rv;

  // we try to drop data not coming with the akonadi:// url
  // find a type the target collection supports
  foreach ( const QString type, mimeData->formats() ) {
    if ( !collection.contentTypes().contains( type ) )
      continue;

    QByteArray item = mimeData->data( type );
    // HACK for some unknown reason the data is sometimes 0-terminated...
    if ( !item.isEmpty() && item.at( item.size() - 1 ) == 0 )
      item.resize( item.size() - 1 );

    Item it;
    it.setMimeType( type );
    it.addPart( Item::PartBody, item );

    ItemAppendJob *job = new ItemAppendJob( it, collection );
    rv.append( job );
    return rv;
  }

  if ( !KUrl::List::canDecode( mimeData ) )
    return rv;

  // data contains an url list
  KUrl::List urls = KUrl::List::fromMimeData( mimeData );
  foreach ( const KUrl url, urls ) {
    if ( Collection::urlIsValid( url ) ) {
      Collection col = Collection::fromUrl( url );
      if ( !copy ) {
        CollectionModifyJob *job = new CollectionModifyJob( col );
        job->setParent( collection );
        rv.append( job );
      } else {
        CollectionCopyJob *job = new CollectionCopyJob( col, collection );
        rv.append( job );
      }
    } else if ( Item::urlIsValid( url ) ) {
      // TODO Extract mimetype from url and check if collection accepts it
      DataReference ref = Item::fromUrl( url );
      if ( !copy ) {
        ItemStoreJob *job = new ItemStoreJob( Item( ref ) );
        job->setCollection( collection );
        job->noRevCheck();
        rv.append( job );
      } else  {
        ItemCopyJob *job = new ItemCopyJob( Item( ref ), collection );
        rv.append( job );
      }
    } else {
      // non-akonadi URL
      // TODO
    }
  }
  return rv;
}

