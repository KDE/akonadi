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

#ifndef PIM_ITEMLISTJOB_H
#define PIM_ITEMLISTJOB_H

#include <libakonadi/job.h>
#include <libakonadi/message.h>

namespace PIM {

class ItemListJobPrivate;

/**
  Fetches message data from the backend.
*/
class AKONADI_EXPORT ItemListJob : public Job
{
    Q_OBJECT
  public:
    /**
      Create a new item list job to retrieve handles to all
      items in the given collection.
      @param path Absolute path of the collection.
      @param parent The parent object.
    */
    ItemListJob( const QByteArray &path, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    virtual ~ItemListJob();

    /**
      Returns the fetched item objects. Invalid before the done(PIM::Job*) signal
      has been emitted or if an error occured.
    */
    Item::List items() const;

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

    /**
      Returns the reference from the given fetch response.
      @param fetchResponse The IMAP fetch response, already splitted into
      name/value pairs
    */
    DataReference parseUid( const QList<QByteArray> &fetchResponse );

  private slots:
    void selectDone( PIM::Job* job );

  private:
    ItemListJobPrivate *d;
};

}

#endif
