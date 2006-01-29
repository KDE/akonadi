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

#ifndef PIM_COLLECTION_JOB
#define PIM_COLLECTION_JOB

#include <libakonadi/job.h>

namespace PIM {

class Collection;

/**
  This class can be used to retrieve the complete or partial collection tree
  of the PIM storage service.

  It returns a QHash of references to PIM::Collection objects.

  @todo Add partial collection retrieval (eg. only collections containing contacts).
*/
class CollectionListJob : public Job
{
  Q_OBJECT

  public:
    /**
      Create a new CollectionListJob.
    */
    CollectionListJob();

    /**
      Destroys this job.
    */
    virtual ~CollectionListJob();

    /**
      Returns a QHash that maps references to PIM::Collection objects.
    */
    QHash<DataReference, Collection*> collections() const;

  private:
    virtual void doStart();

  private slots:
    // just for testing
    void emitDone();
};

}

#endif
