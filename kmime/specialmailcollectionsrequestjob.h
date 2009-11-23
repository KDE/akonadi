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

#ifndef AKONADI_SPECIALMAILCOLLECTIONSREQUESTJOB_H
#define AKONADI_SPECIALMAILCOLLECTIONSREQUESTJOB_H

#include "akonadi-kmime_export.h"

#include <akonadi/specialcollectionsrequestjob.h>
#include <akonadi/kmime/specialmailcollections.h>

namespace Akonadi {

class SpecialMailCollectionsRequestJobPrivate;

/**
 * @short A job to request SpecialMailCollections.
 *
 * Use this job to request the SpecialMailCollections you need. You can request both
 * default SpecialMailCollections and SpecialMailCollections in a given resource. The default
 * SpecialMailCollections resource is created when the first default SpecialCollection is
 * requested, but if a SpecialCollection in a custom resource is requested, this
 * job expects that resource to exist already.
 *
 * If the folders you requested already exist, this job simply succeeds.
 * Otherwise, it creates the required collections and registers them with
 * SpecialMailCollections.
 *
 * Example:
 *
 * @code
 *
 * SpecialMailCollectionsRequestJob *job = new SpecialMailCollectionsRequestJob( this );
 * job->requestDefaultCollection( SpecialMailCollections::Outbox );
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
 *   SpecialMailCollectionsRequestJob *requestJob = qobject_cast<SpecialMailCollectionsRequestJob*>( job );
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
class AKONADI_KMIME_EXPORT SpecialMailCollectionsRequestJob : public SpecialCollectionsRequestJob
{
  Q_OBJECT

  public:
    /**
     * Creates a new special mail collections request job.
     */
    explicit SpecialMailCollectionsRequestJob( QObject *parent = 0 );

    /**
     * Destroys the special mail collections request job.
     */
    ~SpecialMailCollectionsRequestJob();

    /**
     * Requests a special mail collection of the given @p type in the default resource.
     */
    void requestDefaultCollection( SpecialMailCollections::Type type );

    /**
     * Requests a special mail collection of the given @p type in the given resource @p instance.
     */
    void requestCollection( SpecialMailCollections::Type type, const AgentInstance &instance );

  private:
    //@cond PRIVATE
    friend class SpecialMailCollectionsRequestJobPrivate;

    SpecialMailCollectionsRequestJobPrivate *const d;
    //@endcond
};

} // namespace Akonadi

#endif // AKONADI_SPECIALMAILCOLLECTIONSREQUESTJOB_H
