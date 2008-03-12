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

#include "libakonadi_export.h"

#include <kjob.h>

#include <QtGui/qwindowdefs.h>

namespace Akonadi {

/**
  Takes care of creating and (optionally) configuring a new agent instance.
*/
class AKONADI_EXPORT AgentInstanceCreateJob : public KJob
{
  Q_OBJECT
  public:
    /**
      Create a new agent instance creation job.
      @param typeIdentifier The type of the agent to create.
      @param parent The parent object.
    */
    explicit AgentInstanceCreateJob( const QString &typeIdentifier, QObject *parent = 0 );

    /**
      Destructor.
    */
    ~AgentInstanceCreateJob();

    /**
      Show agent configuration dialog once it has been successfully started.
      @param windowId The parent window id for the configuration dialog.
    */
    void configure( WId windowId = 0 );

    /**
      Returns the instance identifier of the newly created agent instance.
    */
    QString instanceIdentifier() const;

    void start();

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void agentInstanceAdded( const QString &id ) )
    Q_PRIVATE_SLOT( d, void doConfigure() )
    Q_PRIVATE_SLOT( d, void timeout() )
};

}

#endif
