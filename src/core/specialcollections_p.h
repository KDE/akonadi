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

#include <QtCore/QHash>
#include <QtCore/QString>

#include "akonadiprivate_export.h"

#include "collection.h"
#include "collectionstatistics.h"
#include "item.h"

class KCoreConfigSkeleton;
class KJob;

namespace Akonadi {

class AgentInstance;
class SpecialCollections;
class Monitor;

/**
  @internal
*/
class AKONADI_TESTS_EXPORT SpecialCollectionsPrivate
{
public:
    SpecialCollectionsPrivate(KCoreConfigSkeleton *settings, SpecialCollections *qq);
    ~SpecialCollectionsPrivate();

    QString defaultResourceId() const;
    void emitChanged(const QString &resourceId);
    void collectionRemoved(const Collection &collection);   // slot
    void collectionFetchJobFinished(KJob *job);  // slot
    void collectionStatisticsChanged(Akonadi::Collection::Id collectionId,
                                     const Akonadi::CollectionStatistics &statistics);  // slot

    /**
      Forgets all folders owned by the given resource.
      This method is used by SpecialCollectionsRequestJob.
      @param resourceId the identifier of the resource for which to forget folders
    */
    void forgetFoldersForResource(const QString &resourceId);

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

    SpecialCollections *q;
    KCoreConfigSkeleton *mSettings;
    QHash<QString, QHash<QByteArray, Collection> > mFoldersForResource;
    bool mBatchMode;
    QSet<QString> mToEmitChangedFor;
    Monitor *mMonitor;

    mutable QString mDefaultResourceId;
};

} // namespace Akonadi

#endif // AKONADI_SPECIALCOLLECTIONS_P_H
