/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include <libakonadi/job.h>
#include <libakonadi/item.h>

namespace Akonadi {

class ItemFetchJobPrivate;

/**
  Fetches message data from the backend.
*/
class AKONADI_EXPORT ItemFetchJob : public Job
{
    Q_OBJECT
  public:
    /**
      Create a new item list job to retrieve handles to all
      items in the given collection.
      @param path Absolute path of the collection.
      @param parent The parent object.
    */
    ItemFetchJob( const QString &path, QObject *parent = 0 );

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
      Returns the fetched item objects. Invalid before the done(Akonadi::Job*)
      signal has been emitted or if an error occurred. Also useless if you are
      using a sub-class.
    */
    Item::List items() const;

    /**
      Fetch additional field.
      @param fields Additional fields to fetch.
    */
    void addFetchField( const QByteArray &field );

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
    void parseFlags( const QByteArray &flagData, Item* item );

  private:
    void startFetchJob();

  private Q_SLOTS:
    void selectDone( Akonadi::Job* job );

  private:
    ItemFetchJobPrivate *d;
};

}

#endif
