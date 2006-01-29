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

#ifndef PIM_COLLECTIONMONITORJOB_H
#define PIM_COLLECTIONMONITORJOB_H

#include <libakonadi/job.h>

namespace PIM {

/**
  Monitors the collection tree and signals all changes.

  @todo Support partial monitoring (eg. monitor only collections containing
  contacts.
*/
class CollectionMonitorJob : public Job
{
  Q_OBJECT

  public:
    /**
      Creates a new collection monitor job.
    */
    CollectionMonitorJob();

    /**
      Destroys this job.
    */
    ~CollectionMonitorJob();

  signals:
    /**
      Emitted when a collection on the storage service has been added or changed.
    */
    void collectionChanged( const DataReference &ref );

    /**
      Emitted if a collection on the storage service has been deleted.
    */
    void collectionRemoved( const DataReference &ref );

  private:
    virtual void doStart();

};

}

#endif
