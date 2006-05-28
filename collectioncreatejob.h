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

#ifndef PIM_COLLECTIONCREATEJOB_H
#define PIM_COLLECTIONCREATEJOB_H

#include <libakonadi/job.h>

namespace PIM {

class CollectionCreateJobPrivate;

/**
  Job to create collections.
*/
class AKONADI_EXPORT CollectionCreateJob : public Job
{
  Q_OBJECT
  public:
    /**
      Create a new CollectionCreateJob job.
      @param path The unique IMAP path of the new collection.
      @param parent The parent object.
    */
    CollectionCreateJob( const QByteArray &path, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    virtual ~CollectionCreateJob();

  protected:
    virtual void doStart();
    virtual void handleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    CollectionCreateJobPrivate *d;
};

}

#endif
