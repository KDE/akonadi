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

#ifndef AKONADI_SPECIALCOLLECTIONSREQUESTJOB_H
#define AKONADI_SPECIALCOLLECTIONSREQUESTJOB_H

#include "akonadi-kmime_export.h"

#include <akonadi/collection.h>
#include <akonadi/kmime/specialcollections.h>
#include <akonadi/transactionsequence.h>

namespace Akonadi {

class SpecialCollectionsRequestJobPrivate;

/**
 * @short A job to request SpecialCollections.
 *
 * Use this job to request the SpecialCollections you need. You can request both
 * default SpecialCollections and SpecialCollections in a given resource. The default
 * SpecialCollections resource is created when the first default SpecialCollection is
 * requested, but if a SpecialCollection in a custom resource is requested, this
 * job expects that resource to exist already.
 *
 * If the folders you requested already exist, this job simply succeeds.
 * Otherwise, it creates the required collections and registers them with
 * SpecialCollections.
 *
 * Example:
 *
 * @code
 *
 * SpecialCollectionsRequestJob *job = new SpecialCollectionsRequestJob( this );
 * job->requestDefaultCollection( SpecialCollections::Outbox );
 * connect( job, SIGNAL( result( KJob* ) ),
 *          this, SLOT( requestDone( KJob* ) ) );
 *
 * ...
 *
 * MyClass::requestDone( KJob *job )
 * {
 *   if ( job->error() )
 *     return;
 *
 *   SpecialCollectionsRequestJob *requestJob = qobject_cast<SpecialCollectionsRequestJob*>( job );
 *
 *   const Collection collection = requestJob->collection();
 *   ...
 * }
 *
 * @endcode
 *
 * @author Constantin Berzan <exit3219@gmail.com>
 * @since 4.4
*/
class AKONADI_KMIME_EXPORT SpecialCollectionsRequestJob : public TransactionSequence
{
  Q_OBJECT

  public:
    /**
     * Creates a new special collections request job.
     */
    explicit SpecialCollectionsRequestJob( QObject *parent = 0 );

    /**
     * Destroys the special collections request job.
     */
    ~SpecialCollectionsRequestJob();

    /**
     * Requests a special collection of the given @p type in the default resource.
     */
    void requestDefaultCollection( SpecialCollections::Type type );

    /**
     * Requests a special collection of the given @p type in the given resource @p instance.
     */
    void requestCollection( SpecialCollections::Type type, const AgentInstance &instance );

    /**
     * Returns the requested collection.
     */
    Collection collection() const;

  protected:
    /* reimpl */
    virtual void doStart();
    /* reimpl */
    virtual void slotResult( KJob *job );

  private:
    //@cond PRIVATE
    friend class SpecialCollectionsRequestJobPrivate;

    SpecialCollectionsRequestJobPrivate *const d;

    Q_PRIVATE_SLOT( d, void lockResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void releaseLock() )
    Q_PRIVATE_SLOT( d, void resourceScanResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void collectionCreateResult( KJob* ) )
    //@endcond
};

} // namespace Akonadi

#endif // AKONADI_SPECIALCOLLECTIONSREQUESTJOB_H
