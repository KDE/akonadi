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
    virtual ~Agent() { };

  public Q_SLOTS:

    /**
     * This method is called to quit the agent.
     */
    virtual void quit() = 0;

    /**
     * This method is called whenever the agent shall show its configuration dialog
     * to the user.
     */
    virtual Q_NOREPLY void configure() = 0;

    /**
     * This method is used to set the name of the agent.
     */
    virtual void setName( const QString &name ) = 0;

    /**
     * Returns the name of the agent.
     */
    virtual QString name() const = 0;

    /**
     * This method is called when the agent is removed from
     * the system, so it can do some cleanup stuff.
     */
    virtual void cleanup() const = 0;

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the name of the resource has changed.
     *
     * @param name The new name of the resource.
     */
    void nameChanged( const QString &name );
};

}

#endif
