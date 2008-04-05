/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_AGENTINSTANCECREATEJOB_H
#define AKONADI_AGENTINSTANCECREATEJOB_H

#include "akonadi_export.h"

#include <akonadi/agenttype.h>

#include <kjob.h>

namespace Akonadi {

class AgentInstance;

/**
  Takes care of creating and (optionally) configuring a new agent instance.
*/
class AKONADI_EXPORT AgentInstanceCreateJob : public KJob
{
  Q_OBJECT
  public:
    /**
      Create a new agent instance creation job.
      @param type The type of the agent to create.
      @param parent The parent object.
    */
    explicit AgentInstanceCreateJob( const AgentType &type, QObject *parent = 0 );

    /**
      Destroys the agent instance creation job.
    */
    ~AgentInstanceCreateJob();

    /**
      Show agent configuration dialog once it has been successfully started.
      @param parent The parent window for the configuration dialog.
    */
    void configure( QWidget *parent = 0 );

    /**
      Returns the instance of the newly created agent instance.
    */
    AgentInstance instance() const;

    /**
     * Starts the instance creation.
     */
    void start();

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void agentInstanceAdded( const Akonadi::AgentInstance& ) )
    Q_PRIVATE_SLOT( d, void doConfigure() )
    Q_PRIVATE_SLOT( d, void timeout() )
    Q_PRIVATE_SLOT( d, void emitResult() )
};

}

#endif
