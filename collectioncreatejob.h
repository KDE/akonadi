/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include <libakonadi/collection.h>
#include <libakonadi/job.h>

namespace Akonadi {

class CollectionCreateJobPrivate;

/**
  Job to create collections.

  @todo Support setting the remote id.
*/
class AKONADI_EXPORT CollectionCreateJob : public Job
{
  Q_OBJECT
  public:
    /**
      Create a new CollectionCreateJob job.
      @param parentCollection The parent collection.
      @param name The collection name
      @param parent The parent object.
    */
    CollectionCreateJob( const Collection &parentCollection, const QString &name, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    virtual ~CollectionCreateJob();

    /**
      Set allowed content mimetypes of the newly created collection.
      @param contentTypes The allowed content types of the new collection.
    */
    void setContentTypes( const QStringList &contentTypes );

    /**
      Sets the remote id of the collection.
      @param remoteId The remote identifier of the collection
    */
    void setRemoteId( const QString &remoteId );

    /**
      Returns the created collection if the job was executed succesfull.
    */
    Collection collection() const;

    /**
      Sets the given collection attribute.
      @param attr A collection attribute, ownership stays with the caller.
    */
    void setAttribute( CollectionAttribute* attr );

    /**
      Sets the cache policy identifier.
      @param cachePolicyId The cache policy identifier, -1 is the default
      (inheriting cache policy from the parent).
    */
    void setCachePolicyId( int cachePolicyId );

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    CollectionCreateJobPrivate* const d;
};

}

#endif
