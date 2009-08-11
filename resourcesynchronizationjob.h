/*
 * Copyright (c) 2009 Volker Krause <vkrause@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AKONADI_RESOURCESYNCJOB_H
#define AKONADI_RESOURCESYNCJOB_H

#include "akonadi_export.h"
#include <kjob.h>

namespace Akonadi {

class AgentInstance;
class ResourceSynchronizationJobPrivate;

/**
  Synchronizes a given resource. If you only want to trigger a
  resource synchronization without being interested in the result,
  using Akonadi::AgentInstance::synchronize() is enough. If you want
  to wait until it's finished, this job is for you.

  @note This is a KJob not an Akonadi::Job, so it wont auto-start!

  @since 4.4
*/
class AKONADI_EXPORT ResourceSynchronizationJob : public KJob
{
  Q_OBJECT
  public:
    /**
      Create a new synchronization job for the given agent.
      @param instance The resource instance to synchronize.
    */
    ResourceSynchronizationJob( const AgentInstance &instance, QObject *parent = 0 );

    /**
      Destructor.
    */
    ~ResourceSynchronizationJob();

    /* reimpl */
    void start();

  private:
    ResourceSynchronizationJobPrivate* const d;
    friend class ResourceSynchronizationJobPrivate;

    Q_PRIVATE_SLOT( d, void slotSynchronized() )
    Q_PRIVATE_SLOT( d, void slotTimeout() )
};

}

#endif
