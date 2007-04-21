/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_MONITOR_P_H
#define AKONADI_MONITOR_P_H

#include "collection.h"
#include "item.h"
#include "job.h"

namespace Akonadi {

/**
  A job which fetches a job and a collection.
 */
class AKONADI_EXPORT ItemCollectionFetchJob : public Job
{
  Q_OBJECT

  public:
    explicit ItemCollectionFetchJob( const DataReference &reference, int collectionId, QObject *parent = 0 );
    ~ItemCollectionFetchJob();

    Item item() const;
    Collection collection() const;

  protected:
    virtual void doStart();

  private Q_SLOTS:
    void collectionJobDone( KJob* job );
    void itemJobDone( KJob* job );

  private:
    DataReference mReference;
    int mCollectionId;

    Item mItem;
    Collection mCollection;
};

}

#endif
