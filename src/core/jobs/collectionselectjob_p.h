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

#ifndef AKONADI_COLLECTIONSELECTJOB_P_H
#define AKONADI_COLLECTIONSELECTJOB_P_H

#include "akonadicore_export.h"
#include "collection.h"
#include "job.h"

namespace Akonadi {

class CollectionSelectJobPrivate;

/**
 * @internal
 *  Selects a specific collection. See RFC 3501 for select semantics.
 */
class AKONADICORE_EXPORT CollectionSelectJob : public Job
{
    Q_OBJECT
public:
    /**
      Creates a new collection select job. When providing a collection with just a remote
      identifier, make sure to specify in which resource to search for that using
      Akonadi::ResourceSelectJob.
      @param collection The collection to select.
      @param parent The parent object.
    */
    explicit CollectionSelectJob(const Collection &collection, QObject *parent = 0);

    /**
      Destroys this job.
    */
    virtual ~CollectionSelectJob();

protected:
    void doStart() Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(CollectionSelectJob)
};

}

#endif
