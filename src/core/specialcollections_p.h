/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QHash>
#include <QString>

#include "akonaditests_export.h"

#include "collection.h"
#include "collectionstatistics.h"
#include "item.h"

class KCoreConfigSkeleton;
class KJob;

namespace Akonadi
{
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

    Q_REQUIRED_RESULT QString defaultResourceId() const;
    void emitChanged(const QString &resourceId);
    void collectionRemoved(const Collection &collection); // slot
    void collectionFetchJobFinished(KJob *job); // slot
    void collectionStatisticsChanged(Akonadi::Collection::Id collectionId,
                                     const Akonadi::CollectionStatistics &statistics); // slot

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

    Q_REQUIRED_RESULT AgentInstance defaultResource() const;

    SpecialCollections *const q;
    KCoreConfigSkeleton *mSettings = nullptr;
    QHash<QString, QHash<QByteArray, Collection>> mFoldersForResource;
    bool mBatchMode;
    QSet<QString> mToEmitChangedFor;
    Monitor *mMonitor = nullptr;

    mutable QString mDefaultResourceId;
};

} // namespace Akonadi
