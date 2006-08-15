/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#ifndef AGENTINSTANCEVIEW_H
#define AGENTINSTANCEVIEW_H

#include <QtGui/QWidget>

namespace PIM {

/**
 * This class provides a view of all available agent instances.
 *
 * Since the view is listening to the dbus for changes, the
 * view is updated automatically as soon as a new agent instance
 * is added or removed to/from the system.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AgentInstanceView : public QWidget
{
  Q_OBJECT

  public:
    /**
     * Creates a new agent instance view.
     *
     * @param parent The parent widget.
     */
    AgentInstanceView( QWidget *parent = 0 );

    /**
     * Destroys the agent instance view.
     */
    ~AgentInstanceView();

    /**
     * Returns the identifier of the current agent instance or
     * an empty string if no agent instance is selected.
     */
    QString currentAgentInstance() const;

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the current agent instance changes.
     *
     * @param current The identifier of the current agent instance.
     * @param previous The identifier of the previous agent instance.
     */
    void currentChanged( const QString &current, const QString &previous );

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void currentAgentInstanceChanged( const QModelIndex&, const QModelIndex& ) )
};

}

#endif
