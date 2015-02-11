/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2008 Omat Holding B.V. <info@omat.nl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef AKONADI_AGENTTYPEDIALOG_H
#define AKONADI_AGENTTYPEDIALOG_H

#include "agenttypewidget.h"
#include "agenttype.h"

#include <QDialog>

namespace Akonadi {

/**
 * @short A dialog to select an available agent type.
 *
 * This dialogs allows the user to select an agent type from the
 * list of all available agent types. The list can be filtered
 * by the proxy model returned by agentFilterProxyModel().
 *
 * @code
 *
 * Akonadi::AgentTypeDialog dlg( this );
 *
 * // only list agent types that provide contacts
 * dlg.agentFilterProxyModel()->addMimeTypeFilter( "text/directory" );
 *
 * if ( dlg.exec() ) {
 *   const AgentType agentType = dlg.agentType();
 *   ...
 * }
 *
 * @endcode
 *
 * @author Tom Albers <tomalbers@kde.nl>
 * @since 4.2
 */
class AKONADIWIDGETS_EXPORT AgentTypeDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Creates a new agent type dialog.
     *
     * @param parent The parent widget of the dialog.
     */
    AgentTypeDialog(QWidget *parent = Q_NULLPTR);

    /**
     * Destroys the agent type dialog.
     */
    ~AgentTypeDialog();

    /**
     * Returns the agent type that was selected by the user,
     * or an empty agent type object if no agent type has been selected.
     */
    AgentType agentType() const;

    /**
     * Returns the agent filter proxy model that can be used
     * to filter the agent types that shall be shown in the
     * dialog.
     */
    AgentFilterProxyModel *agentFilterProxyModel() const;

public Q_SLOTS:
    virtual void done(int result) Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
