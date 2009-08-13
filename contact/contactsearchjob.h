/*
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

#ifndef AKONADI_CONTACTSEARCHJOB_H
#define AKONADI_CONTACTSEARCHJOB_H

#include "akonadi-contact_export.h"

#include <akonadi/item.h>
#include <kabc/addressee.h>
#include <kjob.h>

namespace Akonadi {

/**
 * @short Job that searches for contacts in the Akonadi storage.
 *
 * This job searches for contacts that match given search criteria and returns
 * the list of contacts.
 *
 * @code
 *
 * Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob();
 * job->
 * connect( job, SIGNAL( result( KJob* ) ), this, SLOT( searchResult( KJob* ) ) );
 *
 * ...
 *
 * MyClass::searchResult( KJob *job )
 * {
 *   Akonadi::ContactSearchJob *searchJob = qobject_cast<Akonadi::ContactSearchJob*>( job );
 *   const KABC::Addressee::List contacts = searchJob->contact();
 *   // do something with the contacts
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADI_CONTACT_EXPORT ContactSearchJob : public KJob
{
  Q_OBJECT

  public:
    /**
     * Creates a new contact search job.
     *
     * @param parent The parent object.
     */
    explicit ContactSearchJob( QObject *parent = 0 );

    /**
     * Destroys the contact search job.
     */
    ~ContactSearchJob();

    /**
     * Describes the criteria that can be searched for.
     */
    enum Criterion
    {
      Name, ///< The name of the contact.
      Email ///< The email address of the contact.
    };

    /**
     * Sets the @p criterion and @p value for the search.
     */
    void setQuery( Criterion criterion, const QString &value );

    /**
     * Returns the contacts that matched the search criteria.
     */
    KABC::Addressee::List contacts() const;

    /**
     * Returns the items that matched the search criteria.
     *
     * @note The items only contain the uid but no payload.
     */
    Item::List items() const;

    /**
     * Starts the search job.
     */
    virtual void start();

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void searchResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void fetchResult( KJob* ) )
    //@endcond
};

}

#endif
