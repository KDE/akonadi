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

#ifndef PIM_MESSAGEQUERY_H
#define PIM_MESSAGEQUERY_H

#include <libakonadi/job.h>
#include <libakonadi/message.h>

namespace PIM {

/**
  Retrieving of (partial) messages matching a given query pattern.
*/
class MessageQuery : public Job
{
  Q_OBJECT

  public:
    /**
      Creates a new MessageQuery.

      @param query The query pattern.
      @param part The parts of the data that should be returned
    */
    MessageQuery( const QString &query, const QString &part = QLatin1String( "ALL" ) );

    /**
      Destroys the MessageQuery.
    */
    virtual ~MessageQuery();

    /**
      Returns the list of all matching message objects.
    */
    Message::List messages() const;

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
