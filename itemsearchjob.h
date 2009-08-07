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

#ifndef AKONADI_ITEMSEARCHJOB_H
#define AKONADI_ITEMSEARCHJOB_H

#include <akonadi/item.h>
#include <akonadi/job.h>

namespace Akonadi {

class ItemSearchJobPrivate;

/**
 * @short Job that searches for items in the Akonadi storage.
 *
 * This job searches for items that match a given search query and returns
 * the list of item ids.
 *
 * @code
 *
 * const QString query = "..."; // some sparql query
 *
 * Akonadi::ItemSearchJob *job = new Akonadi::ItemSearchJob( query );
 * connect( job, SIGNAL( result( KJob* ) ), this, SLOT( searchResult( KJob* ) ) );
 *
 * ...
 *
 * MyClass::searchResult( KJob *job )
 * {
 *   Akonadi::ItemSearchJob *searchJob = qobject_cast<Akonadi::ItemSearchJob*>( job );
 *   const Akonadi::Item::List items = searchJob->items();
 *   foreach ( const Akonadi::Item &item, items ) {
 *     // fetch the full payload of 'item'
 *   }
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADI_EXPORT ItemSearchJob : public Job
{
  Q_OBJECT

  public:
    /**
     * Creates an item search job.
     *
     * @param query The search query in SPARQL format.
     * @param parent The parent object.
     */
    ItemSearchJob( const QString &query, QObject *parent = 0 );

    /**
     * Destroys the item search job.
     */
    ~ItemSearchJob();

    /**
     * Returns the items that matched the search query.
     *
     * @note The items only contain the uid but no payload.
     */
    Item::List items() const;

  protected:
    void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    Q_DECLARE_PRIVATE( ItemSearchJob )
};

}

#endif
