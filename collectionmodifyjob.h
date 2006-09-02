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

#ifndef PIM_COLLECTIONMODIFYJOB_H
#define PIM_COLLECTIONMODIFYJOB_H

#include <libakonadi/job.h>

namespace PIM {

class CollectionModifyJobPrivate;

/**
  Job to modify properties of existing collections.
*/
class CollectionModifyJob : public Job
{
  Q_OBJECT

  public:
    /**
      Create a new modify job for the given collection.
      @param path Path of the collection to modify
      @param parent The parent object
    */
    CollectionModifyJob( const QByteArray &path, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    ~CollectionModifyJob();

    /**
      Sets the supported content mimetypes of the collection to the given value.
      @param mimeTypes The new supported content mimetypes.
    */
    void setContentTypes( const QList<QByteArray> &mimeTypes );

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    CollectionModifyJobPrivate* const d;
};

}

#endif
