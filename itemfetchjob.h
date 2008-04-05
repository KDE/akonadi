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

#ifndef AKONADI_ITEMFETCHJOB_H
#define AKONADI_ITEMFETCHJOB_H

#include <akonadi/item.h>
#include <akonadi/job.h>

namespace Akonadi {

class Collection;
class ItemFetchJobPrivate;
class ItemFetchScope;

/**
  Fetches item data from the backend.
*/
class AKONADI_EXPORT ItemFetchJob : public Job
{
    Q_OBJECT
  public:
    /**
      Create a new item list job to retrieve envelope parts of all
      items in the given collection.
      @param collection The collection to list.
      @param parent The parent object.
    */
    explicit ItemFetchJob( const Collection &collection, QObject *parent = 0 );

    /**
      Creates a new item fetch job to retrieve all parts of the item
      with the given id in @p item.
      @param parent The parent object.
    */
    explicit ItemFetchJob( const Item &item, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    virtual ~ItemFetchJob();

    /**
      Returns the fetched item objects. Invalid before the result(KJob*)
      signal has been emitted or if an error occurred.
    */
    Item::List items() const;

    /**
      Sets the item fetch scope.

      Controls how much of an item's data is fetched from the server, e.g.
      whether to fetch the full item payload or only metadata.

      @param fetchScope the new scope for item fetch operations

      @see fetchScope()
    */
    void setFetchScope( ItemFetchScope &fetchScope );

    /**
      Returns the item fetch scope.

      Since this returns a reference it can be used to conveniently modify the
      current scope in-place, i.e. by calling a method on the returned reference
      without storing it in a local variable. See the ItemFetchScope documentation
      for an example.

      @return a reference to the current item fetch scope

      @see setFetchScope() for replacing the current item fetch scope
    */
    ItemFetchScope &fetchScope();

  Q_SIGNALS:
    /**
     Emitted when items are received.
    */
    void itemsReceived( const Akonadi::Item::List &items );

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    Q_DECLARE_PRIVATE( ItemFetchJob )

    Q_PRIVATE_SLOT( d_func(), void selectDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void timeout() )
};

}

#endif
