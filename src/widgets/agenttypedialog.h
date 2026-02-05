/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include "agenttypewidget.h"
// AkonadiCore
#include "akonadi/agenttype.h"

#include <QDialog>

#include <memory>

namespace Akonadi
{
class AgentTypeDialogPrivate;

/*!
 * \class Akonadi::AgentTypeDialog
 * \inheaders Akonadi/AgentTypeDialog
 * \inmodule AkonadiWidgets
 *
 * \brief A dialog to select an available agent type.
 *
 * This dialogs allows the user to select an agent type from the
 * list of all available agent types. The list can be filtered
 * by the proxy model returned by agentFilterProxyModel().
 *
 * \code
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
 * \endcode
 *
 * \author Tom Albers <tomalbers@kde.nl>
 * \since 4.2
 */
class AKONADIWIDGETS_EXPORT AgentTypeDialog : public QDialog
{
    Q_OBJECT

public:
    /*!
     * Creates a new agent type dialog.
     *
     * \a parent The parent widget of the dialog.
     */
    explicit AgentTypeDialog(QWidget *parent = nullptr);

    /*!
     * Destroys the agent type dialog.
     */
    ~AgentTypeDialog() override;

    /*!
     * Returns the agent type that was selected by the user,
     * or an empty agent type object if no agent type has been selected.
     */
    [[nodiscard]] AgentType agentType() const;

    /*!
     * Returns the agent filter proxy model that can be used
     * to filter the agent types that shall be shown in the
     * dialog.
     */
    [[nodiscard]] AgentFilterProxyModel *agentFilterProxyModel() const;

public Q_SLOTS:
    /*!
     */
    void done(int result) override;

private:
    std::unique_ptr<AgentTypeDialogPrivate> const d;
};

}
