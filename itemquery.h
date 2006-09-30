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

#ifndef AKONADI_ITEMQUERY_H
#define AKONADI_ITEMQUERY_H

#include <libakonadi/job.h>
#include <libakonadi/item.h>

namespace Akonadi {

/**
  Retrieving of (partial) messages matching a given query pattern.
*/
class ItemQuery : public Job
{
  Q_OBJECT

  public:
    /**
      Creates a new ItemQuery.

      @param query The query pattern.
      @param part The parts of the data that should be returned
    */
    ItemQuery( const QString &query, const QString &part = "ALL" );

    /**
      Destroys the ItemQuery.
    */
    virtual ~ItemQuery();

    /**
      Returns the list of all matching message objects.
    */
    Item::List items() const;

  private:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private slots:
    // just for testing
    void slotEmitDone();

  private:
    class Private;
    Private* d;
};

}

#endif
