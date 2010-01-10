/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ITEMCREATEJOB_H
#define AKONADI_ITEMCREATEJOB_H

#include <akonadi/job.h>

namespace Akonadi {

class Collection;
class Item;
class ItemCreateJobPrivate;

/**
 * @short Job that creates a new item in the Akonadi storage.
 *
 * This job creates a new item with all the set properties in the
 * given target collection.
 *
 * Example:
 *
 * @code
 *
 * // Create a contact item in the root collection
 *
 * KABC::Addressee addr;
 * addr.setNameFromString( "Joe Jr. Miller" );
 *
 * Akonadi::Item item;
 * item.setMimeType( "text/directory" );
 * item.setPayload<KABC::Addressee>( addr );
 *
 * Akonadi::Collection collection = Akonadi::Collection::root();
 *
 * Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( item, collection );
 * connect( job, SIGNAL( result( KJob* ) ), SLOT( jobFinished( KJob* ) ) );
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
class AKONADI_EXPORT ItemCreateJob : public Job
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
    ItemCreateJob( const Item &item, const Collection &collection, QObject *parent = 0 );

    /**
     * Destroys the item create job.
     */
    ~ItemCreateJob();

    /**
     * Returns the created item with the new unique id, or an invalid item if the job failed.
     */
    Item item() const;

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    Q_DECLARE_PRIVATE( ItemCreateJob )
};

}

#endif
