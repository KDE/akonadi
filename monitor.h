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

#ifndef PIM_MONITOR_H
#define PIM_MONITOR_H

#include <libakonadi/job.h>

namespace PIM {

/**
  Monitors a set of objects that match a given query and emits signals if some
  of these objects are changed or removed or new messages that match the query
  are added to the storage backend.

  A monitor job will run until it is killed or deleted, the done() signal will
  only be emitted in case of a fatal error.
*/
class Monitor : public Job
{
  Q_OBJECT

  public:
    /**
      Creates a new monitor job.
      @param query This query defines which messages are monitored.
      @param parent The parent object.
    */
    Monitor( const QString &query, QObject *parent = 0 );

    /**
      Destroys this monitor job.
    */
    virtual ~Monitor();

  signals:
    /**
      Emitted if some objects have changed.
      @param references A list of references of changed objects.
    */
    void changed( const DataReference::List &references );

    /**
      Emitted if some objects that match the query have been added.
      @param references A list of refereces of added objects.
    */
    void added( const DataReference::List &references );

    /**
      Emitted if some objects have been removed.
      @param references A list of references of removed objects.
    */
    void removed( const DataReference::List &references );

  private:
    virtual void doStart();

  private:
    class Private;
    Private* d;
};

}

#endif
