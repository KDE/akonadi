/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONSTATUSJOB_H
#define AKONADI_COLLECTIONSTATUSJOB_H

#include "libakonadi_export.h"
#include <libakonadi/collection.h>
#include <libakonadi/collectionstatus.h>
#include <libakonadi/job.h>

namespace Akonadi {

class CollectionAttribute;
class CollectionStatusJobPrivate;

/**
  Fetches the CollectionStatus object for a given collection.
*/
class AKONADI_EXPORT CollectionStatusJob : public Job
{
  Q_OBJECT

  public:
    /**
      Creates a new collection status job.
      @param collection The collection.
      @param parent The parent object.
    */
    explicit CollectionStatusJob( const Collection &collection, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    virtual ~CollectionStatusJob();

    /**
      Returns the fetched collection status.
    */
    CollectionStatus status() const;

    /**
      Returns the corresponding collection, if the job was executed successfully,
      the collection is already updated.
    */
    Collection collection() const;

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    CollectionStatusJobPrivate* const d;
};

}

#endif
