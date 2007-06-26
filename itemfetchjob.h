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

#include <libakonadi/collection.h>
#include <libakonadi/job.h>
#include <libakonadi/item.h>

namespace Akonadi {

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
      with the given id.
      @param ref The unique message id.
      @param parent The parent object.
    */
    explicit ItemFetchJob( const DataReference &ref, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    virtual ~ItemFetchJob();

    /**
      Returns the fetched item objects. Invalid before the result(KJob*)
      signal has been emitted or if an error occurred.
    */
    virtual Item::List items() const;

    /**
      Sets the collection that should be listed.
      @param collection The collection that should be listed.
    */
    void setCollection( const Collection &collection );

    /**
      Sets the unique identifier of the item that should be fetched.
      @param ref The unique message id.
    */
    void setUid( const DataReference &ref );

    /**
      Choose which part(s) of the item shall be fetched.
    */
    void addFetchPart( const QString &identifier );

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

    /**
      Returns an item with uid, rid and mimetype set. Ie. basically everything
      the item de-serializer needs to start its work.
      @param fetchResponse The IMAP fetch response, already split into
      name/value pairs.
    */
    Item createItem( const QList<QByteArray> &fetchResponse );

    /**
      Parses the given flag data and sets the them on the given item.
      @param flagData The unparsed flag data of the fetch response
      @param item The corresponding item
    */
    void parseFlags( const QByteArray &flagData, Item &item );

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void selectDone( KJob* ) )
};

}

#endif
