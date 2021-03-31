/*
    SPDX-FileCopyrightText: 2007, 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "collection.h"
#include "transactionsequence.h"

namespace Akonadi
{
/**
  @internal

  Syncs remote and local collections.

  Basic terminology:
  - "local": The current state in the Akonadi server
  - "remote": The state in the backend, which is also the state the Akonadi
              server is supposed to have afterwards.

  There are three options to influence the way syncing is done:
  - Streaming vs. complete delivery: If streaming is enabled remote collections
    do not need to be delivered in a single batch but can be delivered in multiple
    chunks. This improves performance but requires an explicit notification
    when delivery has been completed.
  - Incremental vs. non-incremental: In the incremental case only remote changes
    since the last sync have to be delivered, in the non-incremental mode the full
    remote state has to be provided. The first is obviously the preferred way,
    but requires support by the backend.
  - Hierarchical vs. global RIDs: The first requires RIDs to be unique per parent
    collection, the second one requires globally unique RIDs (per resource). Those
    have different advantages and disadvantages, esp. regarding moving. Which one
    to chose mostly depends on what the backend provides in this regard.

*/
class AKONADICORE_EXPORT CollectionSync : public Job
{
    Q_OBJECT

public:
    /**
      Creates a new collection synchronzier.
      @param resourceId The identifier of the resource we are syncing.
      @param parent The parent object.
    */
    explicit CollectionSync(const QString &resourceId, QObject *parent = nullptr);

    /**
      Destroys this job.
    */
    ~CollectionSync() override;

    /**
      Sets the result of a full remote collection listing.
      @param remoteCollections A list of collections.
      Important: All of these need a unique remote identifier and parent remote
      identifier.
    */
    void setRemoteCollections(const Collection::List &remoteCollections);

    /**
      Sets the result of an incremental remote collection listing.
      @param changedCollections A list of remotely added or changed collections.
      @param removedCollections A list of remotely deleted collections.
    */
    void setRemoteCollections(const Collection::List &changedCollections, const Collection::List &removedCollections);

    /**
      Enables streaming, that is not all collections are delivered at once.
      Use setRemoteCollections() multiple times when streaming is enabled and call
      retrievalDone() when all collections have been retrieved.
      Must be called before the first call to setRemoteCollections().
      @param streaming enables streaming if set as @c true
    */
    void setStreamingEnabled(bool streaming);

    /**
      Indicate that all collections have been retrieved in streaming mode.
    */
    void retrievalDone();

    /**
      Indicate whether the resource supplies collections with hierarchical or
      global remote identifiers. @c false by default.
      Must be called before the first call to setRemoteCollections().
      @param hierarchical @c true if collection remote IDs are relative to their parents' remote IDs
    */
    void setHierarchicalRemoteIds(bool hierarchical);

    /**
      Do a rollback operation if needed. In read only cases this is a noop.
    */
    void rollback();

    /**
     * Allows to specify parts of the collection that should not be changed if locally available.
     *
     * This is useful for resources to provide default values during the collection sync, while
     * preserving more up-to date values if available.
     *
     * Use CONTENTMIMETYPES as identifier to not overwrite the content mimetypes.
     *
     * @since 4.14
     */
    void setKeepLocalChanges(const QSet<QByteArray> &parts);

protected:
    void doStart() override;

private:
    class Private;
    Private *const d;
};

}

