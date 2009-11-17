/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "specialcollectionstesting.h"

#include "specialcollections_p.h"
#include "specialcollectionssettings.h"

using namespace Akonadi;

typedef SpecialCollectionsSettings Settings;

SpecialCollectionsTesting *SpecialCollectionsTesting::_t_self()
{
  static SpecialCollectionsTesting *instance = 0;
  if( !instance ) {
    instance = new SpecialCollectionsTesting;
  }
  return instance;
}

void SpecialCollectionsTesting::_t_setDefaultResourceId( const QString &resourceId )
{
  Settings::setDefaultResourceId( resourceId );
}

void SpecialCollectionsTesting::_t_forgetFoldersForResource( const QString &resourceId )
{
  SpecialCollections::self()->d->forgetFoldersForResource( resourceId );
}

void SpecialCollectionsTesting::_t_beginBatchRegister()
{
  SpecialCollections::self()->d->beginBatchRegister();
}

void SpecialCollectionsTesting::_t_endBatchRegister()
{
  SpecialCollections::self()->d->endBatchRegister();
}

int SpecialCollectionsTesting::_t_knownResourceCount() const
{
  const SpecialCollectionsPrivate *d = SpecialCollections::self()->d;
  return d->foldersForResource.count();
}

int SpecialCollectionsTesting::_t_knownFolderCount() const
{
  const SpecialCollectionsPrivate *d = SpecialCollections::self()->d;
  int ret = 0;
  typedef QHash<SpecialCollections::Type, Collection> CollectionHash;
  foreach( const  CollectionHash &hash, d->foldersForResource ) {
    foreach( const Collection &col, hash.values() ) {
      if( col.isValid() ) {
        ret++;
      }
    }
  }
  return ret;
}
