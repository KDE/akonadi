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

#include "localfolderstesting.h"

#include "localfolders_p.h"
#include "localfolderssettings.h"

using namespace Akonadi;

typedef LocalFoldersSettings Settings;

LocalFoldersTesting *LocalFoldersTesting::_t_self()
{
  static LocalFoldersTesting *instance = 0;
  if( !instance ) {
    instance = new LocalFoldersTesting;
  }
  return instance;
}

void LocalFoldersTesting::_t_setDefaultResourceId( const QString &resourceId )
{
  Settings::setDefaultResourceId( resourceId );
}

void LocalFoldersTesting::_t_forgetFoldersForResource( const QString &resourceId )
{
  LocalFolders::self()->forgetFoldersForResource( resourceId );
}

void LocalFoldersTesting::_t_beginBatchRegister()
{
  LocalFolders::self()->beginBatchRegister();
}

void LocalFoldersTesting::_t_endBatchRegister()
{
  LocalFolders::self()->endBatchRegister();
}

int LocalFoldersTesting::_t_knownResourceCount() const
{
  const LocalFoldersPrivate *d = LocalFolders::self()->d;
  return d->foldersForResource.count();
}

int LocalFoldersTesting::_t_knownFolderCount() const
{
  const LocalFoldersPrivate *d = LocalFolders::self()->d;
  int ret = 0;
  foreach( const Collection::List &list, d->foldersForResource ) {
    foreach( const Collection &col, list ) {
      if( col.isValid() ) {
        ret++;
      }
    }
  }
  return ret;
}
