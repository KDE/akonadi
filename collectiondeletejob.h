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

#ifndef PIM_COLLECTIONDELETEJOB_H
#define PIM_COLLECTIONDELETEJOB_H

#include <libakonadi/job.h>

namespace PIM {

class CollectionDeleteJobPrivate;

/**
  Job to delete collections.
  Be carefull with using this: It deletes not only the specified collection
  but also all its sub-collections as well as all associated content!
*/
class CollectionDeleteJob : public Job
{
  Q_OBJECT

  public:
    /**
      Creates a new CollectionDeleteJob.
      @param path Path of a collection to delete.
      @param parent The parent object.
    */
    CollectionDeleteJob( const QString &path, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    ~CollectionDeleteJob();

  protected:
    virtual void doStart();

  private:
    CollectionDeleteJobPrivate* const d;
};

}

#endif
