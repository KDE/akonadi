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

#ifndef AKONADI_ITEMAPPENDJOB_H
#define AKONADI_ITEMAPPENDJOB_H

#include <libakonadi/collection.h>
#include <libakonadi/item.h>
#include <libakonadi/job.h>

namespace Akonadi {

/**
  Creates a new PIM item on the backend.
*/
class AKONADI_EXPORT ItemAppendJob : public Job
{
  Q_OBJECT
  public:
    /**
      Create a new item append job.
      @param item The item to append. It must have a mimetype set.
      @param collection Parent collection.
      @param parent The parent object.
    */
    ItemAppendJob( const Item &item, const Collection &collection, QObject *parent = 0 );

    /**
      Deletes this job.
    */
    ~ItemAppendJob();

    /**
      Returns a DataReference pointing to the new item. The result is invalid
      if the job failed.
    */
    DataReference reference() const;

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    class Private;
    Private* const d;
};

}

#endif
