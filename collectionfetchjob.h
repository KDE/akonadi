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

#ifndef PIM_COLLECTIONFETCHJOB_H
#define PIM_COLLECTIONFETCHJOB_H

#include <libakonadi/job.h>

namespace PIM {

class Collection;

/**
  Fetches a Collection object for a given DataReference from the storage
  service.

  @todo Derive from DataRequest instead from Job and support fetching lists?
*/
class CollectionFetchJob : public Job
{
  Q_OBJECT

  public:
    /**
      Creates a new collection fetch job.
      @param ref The collection to fetch.
    */
    CollectionFetchJob( const DataReference &ref );

    /**
      Destroys this job.
    */
    virtual ~CollectionFetchJob();

    /**
      Returns the fetched collection, 0 if the collection is not (yet) available
      or if there has been an error.
      It is your responsibility to delete this collection object.
    */
    Collection* collection() const;

  private slots:
    // ### just for testing ###
    void emitDone();

  private:
    void doStart();
};

}

#endif
