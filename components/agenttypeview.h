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

#ifndef AGENTTYPEVIEW_H
#define AGENTTYPEVIEW_H

#include <QtGui/QWidget>
#include <kdepim_export.h>
namespace Akonadi {

/**
 * This class provides a view of all available agent types.
 *
 * Since the view is listening to the dbus for changes, the
 * view is updated automatically as soon as a new agent type
 * is installed or removed to/from the system.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADICOMPONENTS_EXPORT AgentTypeView : public QWidget
{
  Q_OBJECT

  public:
    /**
     * Creates a new agent type view.
     *
     * @param parent The parent widget.
     */
    AgentTypeView( QWidget *parent = 0 );

    /**
     * Destroys the agent type view.
     */
    ~AgentTypeView();

    /**
     * Returns the identifier of the current agent type or an
     * empty string if no agent type is selected.
     */
    QString currentAgentType() const;

  public Q_SLOTS:
    /**
     * Sets a filter to the view, so only agent types which
     * provides the given list of @p mimetypes will be listed
     * by the model.
     */
    void setFilter( const QStringList &mimeTypes );

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the current agent type changes.
     *
     * @param current The identifier of the current agent type.
     * @param previous The identifier of the previous agent type.
     */
    void currentChanged( const QString &current, const QString &previous );

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void currentAgentTypeChanged( const QModelIndex&, const QModelIndex& ) )
};

}

#endif
