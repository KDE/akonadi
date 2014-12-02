/*
  Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_AGENTACTIONMANAGER_H
#define AKONADI_AGENTACTIONMANAGER_H

#include "akonadiwidgets_export.h"
#include "agentinstance.h"

#include <QtCore/QObject>

class QAction;
class KActionCollection;
class KLocalizedString;
class QItemSelectionModel;
class QWidget;

namespace Akonadi {

/**
 * @short Manages generic actions for agent and agent instance views.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.6
 */
class AKONADIWIDGETS_EXPORT AgentActionManager : public QObject
{
    Q_OBJECT
public:
    /**
     * Describes the supported actions.
     */
    enum Type {
        CreateAgentInstance,       ///< Creates an agent instance
        DeleteAgentInstance,       ///< Deletes the selected agent instance
        ConfigureAgentInstance,    ///< Configures the selected agent instance
        LastType                   ///< Marks last action
    };

    /**
     * Describes the text context that can be customized.
     */
    enum TextContext {
        DialogTitle,                ///< The window title of a dialog
        DialogText,                 ///< The text of a dialog
        MessageBoxTitle,            ///< The window title of a message box
        MessageBoxText,             ///< The text of a message box
        MessageBoxAlternativeText,  ///< An alternative text of a message box
        ErrorMessageTitle,          ///< The window title of an error message
        ErrorMessageText            ///< The text of an error message
    };

    /**
     * Creates a new agent action manager.
     *
     * @param actionCollection The action collection to operate on.
     * @param parent The parent widget.
     */
    explicit AgentActionManager(KActionCollection *actionCollection, QWidget *parent = Q_NULLPTR);

    /**
     * Destroys the agent action manager.
     */
    ~AgentActionManager();

    /**
     * Sets the agent selection @p model based on which the actions should operate.
     * If none is set, all actions will be disabled.
     * @param model model based on which actions should operate
     */
    void setSelectionModel(QItemSelectionModel *model);

    /**
     * Sets the mime type filter that will be used when creating new agent instances.
     */
    void setMimeTypeFilter(const QStringList &mimeTypes);

    /**
     * Sets the capability filter that will be used when creating new agent instances.
     */
    void setCapabilityFilter(const QStringList &capabilities);

    /**
     * Creates the action of the given type and adds it to the action collection
     * specified in the constructor if it does not exist yet. The action is
     * connected to its default implementation provided by this class.
     * @param type action type
     */
    QAction *createAction(Type type);

    /**
     * Convenience method to create all standard actions.
     * @see createAction()
     */
    void createAllActions();

    /**
     * Returns the action of the given type, 0 if it has not been created (yet).
     */
    QAction *action(Type type) const;

    /**
     * Sets whether the default implementation for the given action @p type
     * shall be executed when the action is triggered.
     *
     * @param intercept If @c false, the default implementation will be executed,
     *                  if @c true no action is taken.
     *
     * @since 4.6
     */
    void interceptAction(Type type, bool intercept = true);

    /**
     * Returns the list of agent instances that are currently selected.
     * The list is empty if no agent instance is currently selected.
     *
     * @since 4.6
     */
    Akonadi::AgentInstance::List selectedAgentInstances() const;

    /**
     * Sets the @p text of the action @p type for the given @p context.
     *
     * @param type action type
     * @param context context of the given action
     * @param text text for the given action type
     *
     * @since 4.6
     */
    void setContextText(Type type, TextContext context, const QString &text);

    /**
     * Sets the @p text of the action @p type for the given @p context.
     *
     * @since 4.8
     * @param type action type
     * @param context context of the given action type
     * @param text localized text for the given action type
     */
    void setContextText(Type type, TextContext context, const KLocalizedString &text);

Q_SIGNALS:
    /**
     * This signal is emitted whenever the action state has been updated.
     * In case you have special needs for changing the state of some actions,
     * connect to this signal and adjust the action state.
     */
    void actionStateUpdated();

private:
    //@cond PRIVATE
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void updateActions())

    Q_PRIVATE_SLOT(d, void slotCreateAgentInstance())
    Q_PRIVATE_SLOT(d, void slotDeleteAgentInstance())
    Q_PRIVATE_SLOT(d, void slotConfigureAgentInstance())

    Q_PRIVATE_SLOT(d, void slotAgentInstanceCreationResult(KJob *))
    //@endcond
};

}

#endif
