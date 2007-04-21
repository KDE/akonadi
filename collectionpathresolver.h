/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONPATHRESOLVER_H
#define AKONADI_COLLECTIONPATHRESOLVER_H

#include <libakonadi/collection.h>
#include <libakonadi/job.h>

namespace Akonadi {

/**
  Converts between collection id and collection path.

  While it is generally recommended to use collection ids, it can
  be necessary in some cases (eg. a command line client) to use the
  collection path instead. Use this class to get a collection id
  from a collection path.
*/
class AKONADI_EXPORT CollectionPathResolver : public Job
{
  Q_OBJECT

  public:
    /**
      Creates a new CollectionPathResolver to convert a path into a id.
      @param path The collection path.
      @param parent The parent object.
    */
    explicit CollectionPathResolver( const QString &path, QObject *parent = 0 );

    /**
      Creates a new CollectionPathResolver to determine the path of
      the given collection.
      @param collection The collection
      @param parent The parent object.
    */
    explicit CollectionPathResolver( const Collection &collection, QObject *parent = 0 );

    /**
      Destroys this object.
    */
    ~CollectionPathResolver();

    /**
      Returns the collection id. Only valid after the job succeeded.
    */
    int collection() const;

    /**
      Returns the collection path. Only valid after the job succeeded.
    */
    QString path() const;

  protected:
    void doStart();

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void jobResult( KJob* ) )

};

}

#endif
