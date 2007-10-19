/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>

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

#ifndef AKONADI_AGENT_H
#define AKONADI_AGENT_H

#include "libakonadi_export.h"
#include <QtCore/QObject>

#ifndef Q_NOREPLY
#define Q_NOREPLY
#endif


namespace Akonadi {

/**
 * Abstract interface for all agent classes
 * TODO make Resource inherit from this
 */
class AKONADI_EXPORT Agent : public QObject
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.kde.Akonadi.Agent" )

  public:
    typedef QList<Agent*> List;

    /**
     * Destroys the agent.
     */
    virtual ~Agent() { }

  public Q_SLOTS:

    /**
     * This method is called to quit the agent.
     */
    virtual void quit() = 0;

    /**
     * This method is called when the agent is removed from
     * the system, so it can do some cleanup stuff.
     */
    virtual void cleanup() = 0;
};

}

#endif
