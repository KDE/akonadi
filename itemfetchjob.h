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
      Creates a new item fetch job.
      @param parent The parent object.
    */
    explicit ItemFetchJob( QObject *parent = 0 );

    /**
      Create a new item list job to retrieve handles to all
      items in the given collection.
      @param collection The collection to list.
      @param parent The parent object.
    */
    ItemFetchJob( const Collection &collection, QObject *parent = 0 );

    /**
      Creates a new item fetch job to retrieve the complete item data
      with the given uid.
      @param ref The unique message id.
      @param parent The parent object.
    */
    ItemFetchJob( const DataReference &ref, QObject *parent = 0 );

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
      Fetch additional field.
      @param field Additional field to fetch.
    */
    void addFetchField( const QByteArray &field );

    /**
      Choose whether the item data should be fetched.
      @param fetch @c true to fetch the item data, fetch only
      meta-data otherwise.
    */
    void fetchData( bool fetch );

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

    /**
      Returns the reference from the given fetch response.
      @param fetchResponse The IMAP fetch response, already splitted into
      name/value pairs
    */
    DataReference parseUid( const QList<QByteArray> &fetchResponse );

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
