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

#ifndef AKONADI_LOCALFOLDERS_P_H
#define AKONADI_LOCALFOLDERS_P_H

#include <QHash>
#include <QString>

#include <Akonadi/Collection>

class KJob;

namespace Akonadi {

class LocalFolders;
class Monitor;

/**
  @internal
*/
class LocalFoldersPrivate
{
  public:
    LocalFoldersPrivate();
    ~LocalFoldersPrivate();

    void emitChanged( const QString &resourceId );
    void collectionRemoved( const Collection &col ); // slot

    LocalFolders *instance;
    Collection::List emptyFolderList;
    QHash<QString,Collection::List> foldersForResource;
    bool batchMode;
    QSet<QString> toEmitChangedFor;
    Monitor *monitor;
};

} // namespace Akonadi

#endif // AKONADI_LOCALFOLDERS_P_H
