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

#ifndef AKONADI_SPECIALCOLLECTIONS_P_H
#define AKONADI_SPECIALCOLLECTIONS_P_H

#include <QHash>
#include <QString>

#include "akonadi-kmime_export.h"

#include "akonadi/collection.h"

class KJob;

namespace Akonadi {

class AgentInstance;
class SpecialCollections;
class Monitor;

/**
  @internal
*/
class AKONADI_KMIME_TEST_EXPORT SpecialCollectionsPrivate
{
  public:
    SpecialCollectionsPrivate();
    ~SpecialCollectionsPrivate();

    void emitChanged( const QString &resourceId );
    void collectionRemoved( const Collection &col ); // slot

    /**
      Forgets all folders owned by the given resource.
      This method is used by SpecialCollectionsRequestJob.
    */
    void forgetFoldersForResource( const QString &resourceId );

    /**
      Avoids emitting the foldersChanged() signal until endBatchRegister()
      is called. This is used to avoid emitting repeated signals when multiple
      folders are registered in a row.
      This method is used by SpecialCollectionsRequestJob.
    */
    void beginBatchRegister();

    /**
      @see beginBatchRegister()
      This method is used by SpecialCollectionsRequestJob.
    */
    void endBatchRegister();

    AgentInstance defaultResource() const;

    SpecialCollections *instance;
    Collection::List emptyFolderList;
    QHash<QString,Collection::List> foldersForResource;
    bool batchMode;
    QSet<QString> toEmitChangedFor;
    Monitor *monitor;
};

} // namespace Akonadi

#endif // AKONADI_SPECIALCOLLECTIONS_P_H
