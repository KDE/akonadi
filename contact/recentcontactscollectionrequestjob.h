/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_RECENTCONTACTSCOLLECTIONREQUESTJOB_H
#define AKONADI_RECENTCONTACTSCOLLECTIONREQUESTJOB_H

#include "akonadi-contact_export.h"

#include <akonadi/specialcollectionsrequestjob.h>

namespace Akonadi {

class RecentContactsCollectionRequestJobPrivate;

/**
 * @short A job to request the collection used for storing recent contacts.
 *
 * Use this job to retrieve the collection that is used for storing recent contacts
 * (e.g. by email clients).
 *
 * Example:
 *
 * @code
 *
 * RecentContactsCollectionRequestJob *job = new RecentContactsCollectionRequestJob( this );
 * job->requestDefaultCollection();
 * connect( job, SIGNAL( result( KJob* ) ), this, SLOT( requestDone( KJob* ) ) );
 *
 * ...
 *
 * MyClass::requestDone( KJob *job )
 * {
 *   if ( job->error() )
 *     return;
 *
 *   RecentContactsCollectionRequestJob *requestJob = qobject_cast<RecentContactsCollectionRequestJob*>( job );
 *
 *   const Collection collection = requestJob->collection();
 *   ...
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.4
*/
class AKONADI_CONTACT_EXPORT RecentContactsCollectionRequestJob : public SpecialCollectionsRequestJob
{
  Q_OBJECT

  public:
    /**
     * Creates a new recent contacts collection request job.
     */
    explicit RecentContactsCollectionRequestJob( QObject *parent = 0 );

    /**
     * Destroys the recent contacts collection request job.
     */
    ~RecentContactsCollectionRequestJob();

    /**
     * Requests the recent contacts collection in the default resource.
     */
    void requestDefaultCollection();

    /**
     * Requests the recent contacts collection in the given resource @p instance.
     */
    void requestCollection( const AgentInstance &instance );

  private:
    //@cond PRIVATE
    friend class RecentContactsCollectionRequestJobPrivate;

    RecentContactsCollectionRequestJobPrivate *const d;
    //@endcond
};

}

#endif
