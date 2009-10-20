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

#ifndef AKONADI_CONTACTGROUPEXPANDJOB_H
#define AKONADI_CONTACTGROUPEXPANDJOB_H

#include "akonadi-contact_export.h"

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>
#include <kjob.h>

namespace Akonadi {

/**
 * @short Job that expands a ContactGroup to a list of contacts.
 *
 * This job takes a KABC::ContactGroup object and expands it to a list
 * of KABC::Addressee objects by creating temporary KABC::Addressee objects
 * for the KABC::ContactGroup::Data objects of the group and fetching the
 * complete contacts from the Akonadi storage for the
 * KABC::ContactGroup::ContactReferences of the group.
 *
 * @code
 *
 * const KABC::ContactGroup group = ...;
 *
 * Akonadi::ContactGroupExpandJob *job = new Akonadi::ContactGroupExpandJob( group );
 * connect( job, SIGNAL( result( KJob* ) ), this, SLOT( expandResult( KJob* ) ) );
 * job->start();
 *
 * ...
 *
 * MyClass::expandResult( KJob *job )
 * {
 *   Akonadi::ContactGroupExpandJob *expandJob = qobject_cast<Akonadi::ContactGroupExpandJob*>( job );
 *   const KABC::Addressee::List contacts = expandJob->contacts();
 *   // do something with the contacts
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.4
 */
class AKONADI_CONTACT_EXPORT ContactGroupExpandJob : public KJob
{
  Q_OBJECT

  public:
    /**
     * Creates a new contact group expand job.
     *
     * @param group The contact group to expand.
     * @param parent The parent object.
     */
    explicit ContactGroupExpandJob( const KABC::ContactGroup &group, QObject *parent = 0 );

    /**
     * Destroys the contact group expand job.
     */
    ~ContactGroupExpandJob();

    /**
     * Returns the list of contacts.
     */
    KABC::Addressee::List contacts() const;

    /**
     * Starts the expand job.
     */
    virtual void start();

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void fetchResult( KJob* ) )
    //@endcond
};

}

#endif
