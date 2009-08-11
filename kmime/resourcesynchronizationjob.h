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

#include <kjob.h>

namespace Akonadi {

class AgentInstance;
class ResourceSynchronizationJobPrivate;

/**
  Synchronizes a given resource.
*/
class ResourceSynchronizationJob : public KJob
{
  Q_OBJECT
  public:
    /**
      Create a new synchronization job for the given agent.
    */
    ResourceSynchronizationJob( const AgentInstance &instance, QObject *parent = 0 );

    /**
      Destructor.
    */
    ~ResourceSynchronizationJob();

    /* reimpl */
    void start();

  private slots:
    void slotSynchronized();
    void slotTimeout();

  private:
    ResourceSynchronizationJobPrivate* const d;
};

}

#endif
