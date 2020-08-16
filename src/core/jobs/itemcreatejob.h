/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ITEMCREATEJOB_H
#define AKONADI_ITEMCREATEJOB_H

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi
{

class Collection;
class Item;
class ItemCreateJobPrivate;

/**
 * @short Job that creates a new item in the Akonadi storage.
 *
 * This job creates a new item with all the set properties in the
 * given target collection.
 *
 * Note that items can not be created in the root collection (Collection::root())
 * and the collection must have Collection::contentMimeTypes() that match the mimetype
 * of the item being created.
 *
 * Example:
 *
 * @code
 *
 * // Create a contact item in the root collection
 *
 * KContacts::Addressee addr;
 * addr.setNameFromString( "Joe Jr. Miller" );
 *
 * Akonadi::Item item;
 * item.setMimeType( "text/directory" );
 * item.setPayload<KContacts::Addressee>( addr );
 *
 * Akonadi::Collection collection = getCollection();
 *
 * Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( item, collection );
 * connect( job, SIGNAL(result(KJob*)), SLOT(jobFinished(KJob*)) );
 *
 * ...
 *
 * MyClass::jobFinished( KJob *job )
 * {
 *   if ( job->error() )
 *     qDebug() << "Error occurred";
 *   else
 *     qDebug() << "Contact item created successfully";
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT ItemCreateJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new item create job.
     *
     * @param item The item to create.
     *             @note It must have a mime type set.
     * @param collection The parent collection where the new item shall be located in.
     * @param parent The parent object.
     */
    ItemCreateJob(const Item &item, const Collection &collection, QObject *parent = nullptr);

    /**
     * Destroys the item create job.
     */
    ~ItemCreateJob() override;

    /**
     * Returns the created item with the new unique id, or an invalid item if the job failed.
     */
    Q_REQUIRED_RESULT Item item() const;

    enum MergeOption {
        NoMerge = 0, ///< Don't merge
        RID     = 1, ///< Merge by remote id
        GID     = 2, ///< Merge by GID
        Silent  = 4  ///< Only return the id of the merged/created item.
    };
    Q_DECLARE_FLAGS(MergeOptions, MergeOption)

    /**
     * Merge this item into an existing one if available.
     *
     * If an item with same GID and/or remote ID as the created item exists in
     * specified collection (depending on the provided options), the new item will
     * be merged into the existing one and the merged item will be returned
     * (unless the Silent option is used).
     *
     * If no matching item is found a new item is created.
     *
     * If the item does not have a GID or RID, this option will be
     * ignored and a new item will be created.
     *
     * By default, merging is disabled.
     *
     * @param options Merge options.
     * @since 4.14
     */
    void setMerge(MergeOptions options);

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(ItemCreateJob)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ItemCreateJob::MergeOptions)

}

#endif
