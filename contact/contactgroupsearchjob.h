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

#ifndef AKONADI_CONTACTGROUPSEARCHJOB_H
#define AKONADI_CONTACTGROUPSEARCHJOB_H

#include "akonadi-contact_export.h"

#include <akonadi/item.h>
#include <akonadi/itemsearchjob.h>
#include <kabc/contactgroup.h>

namespace Akonadi {

/**
 * @short Job that searches for contact groups in the Akonadi storage.
 *
 * This job searches for contact groups that match given search criteria and returns
 * the list of contact groups.
 *
 * @code
 *
 * Akonadi::ContactGroupSearchJob *job = new Akonadi::ContactGroupSearchJob();
 * job->setQuery( Akonadi::ContactGroupSearchJob::Name, "Family Members" );
 * connect( job, SIGNAL( result( KJob* ) ), this, SLOT( searchResult( KJob* ) ) );
 *
 * ...
 *
 * MyClass::searchResult( KJob *job )
 * {
 *   Akonadi::ContactGroupSearchJob *searchJob = qobject_cast<Akonadi::ContactGroupSearchJob*>( job );
 *   const KABC::ContactGroup::List contactGroups = searchJob->contactGroups();
 *   // do something with the contact groups
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.4
 */
class AKONADI_CONTACT_EXPORT ContactGroupSearchJob : public ItemSearchJob
{
  Q_OBJECT

  public:
    /**
     * Creates a new contact group search job.
     *
     * @param parent The parent object.
     */
    explicit ContactGroupSearchJob( QObject *parent = 0 );

    /**
     * Destroys the contact group search job.
     */
    ~ContactGroupSearchJob();

    /**
     * Describes the criteria that can be searched for.
     */
    enum Criterion
    {
      Name ///< The name of the contact group.
    };

    /**
     * Sets the @p criterion and @p value for the search.
     */
    void setQuery( Criterion criterion, const QString &value );

    /**
     * Sets a @p limit on how many results will be returned by this search job.
     * This is useful in situation where for example only the first search result is needed anyway,
     * setting a limit of 1 here will greatly reduce the resource usage of Nepomuk during the
     * search.
     *
     * This needs to be called before calling setQuery() to have an effect.
     * By default, the number of results is unlimited.
     *
     * @since 4.4.3
     */
    void setLimit( int limit );

    /**
     * Returns the contact groups that matched the search criteria.
     */
    KABC::ContactGroup::List contactGroups() const;

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
