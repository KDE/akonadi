/*
    Copyright (c) 2006-2008 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_AGENTTYPEWIDGET_H
#define AKONADI_AGENTTYPEWIDGET_H

#include "akonadi_export.h"

#include <QtGui/QWidget>

namespace Akonadi {

class AgentFilterProxyModel;
class AgentType;

/**
 * @short Provides a widget that lists all available agent types.
 *
 * The widget is listening on the dbus for changes, so the
 * widget is updated automatically as soon as new agent types
 * are added to or removed from the system.
 *
 * @code
 *
 * MyWidget::MyWidget( QWidget *parent )
 *   : QWidget( parent )
 * {
 *   QVBoxLayout *layout = new QVBoxLayout( this );
 *
 *   mAgentTypeWidget = new Akonadi::AgentTypeWidget( this );
 *   layout->addWidget( mAgentTypeWidget );
 *
 *   connect( mAgentTypeWidget, SIGNAL( currentChanged( Akonadi::AgentType&, Akonadi::AgentType& ) ),
 *            this, SLOT( slotTypeChanged( Akonadi::AgentType& ) ) );
 * }
 *
 * ...
 *
 * MyWidget::slotTypeChanged( Akonadi::AgentType &current, Akonadi::AgentType& )
 * {
 *   qDebug() << "New selected type:" << current.name();
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADI_EXPORT AgentTypeWidget : public QWidget
{
  Q_OBJECT

  public:
    /**
     * Creates a new agent type widget.
     *
     * @param parent The parent widget.
     */
    explicit AgentTypeWidget( QWidget *parent = 0 );

    /**
     * Destroys the agent type widget.
     */
    ~AgentTypeWidget();

    /**
     * Returns the current agent type or an invalid agent type
     * if no agent type is selected.
     */
    AgentType currentAgentType() const;

    /**
     * Returns the agent filter proxy model, use this to filter by
     * agent mimetype or capabilities.
     */
    AgentFilterProxyModel* agentFilterProxyModel() const;

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the current agent type changes.
     *
     * @param current The current agent type.
     * @param previous The previous agent type.
     */
    void currentChanged( const Akonadi::AgentType &current, const Akonadi::AgentType &previous );

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void currentAgentTypeChanged( const QModelIndex&, const QModelIndex& ) )
    //@endcond
};

}

#endif
