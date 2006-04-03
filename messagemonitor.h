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

#ifndef PIM_MESSAGEMONITOR_H
#define PIM_MESSAGEMONITOR_H

#include <libakonadi/job.h>

namespace PIM {

/**
  Monitors a set of messages that match a given query and emits signals if
  these messages are changed or removed or new messages that match the query
  are added to the storage backend.
*/
class MessageMonitor : public Job
{
  Q_OBJECT

  public:
    /**
      Creates a new MessageMonitor job.
      @param query This query defines which messages are monitored.
    */
    MessageMonitor( const QString &query );

    /**
      Destroys this MessageMonitor job.
    */
    virtual ~MessageMonitor();

  signals:
    /**
      Emitted if some messages have changed.
      @param references A list of references of changed messages.
    */
    void messagesChanged( const DataReference::List &refrences );

    /**
      Emitted if some messages that match the query have been added.
      @param references A list of refereces of added messages.
    */
    void messagesAdded( const DataReference::List &references );

    /**
      Emitted if some messages have been removed.
      @param references A list of references of removed messages.
    */
    void messagesRemoved( const DataReference::List &references );

  private:
    virtual void doStart();

  private:
    class Private;
    Private* d;
};

}

#endif
