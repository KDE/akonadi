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

#ifndef AKONADI_COLLECTIONCREATEJOB_H
#define AKONADI_COLLECTIONCREATEJOB_H

#include <libakonadi/job.h>

namespace Akonadi {

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
    CollectionCreateJob( const QString &path, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    virtual ~CollectionCreateJob();

    /**
      Returns the path of the collection this job is supposed to create.
    */
    QString path() const;

    /**
      Set allowed content mimetypes of the newly created collection.
      @param contentTypes The allowed content types of the new collection.
    */
    void setContentTypes( const QList<QByteArray> &contentTypes );

  protected:
    virtual void doStart();

  private:
    CollectionCreateJobPrivate *d;
};

}

#endif
