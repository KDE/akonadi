/*
    SPDX-FileCopyrightText: 2006-2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"

#include <QWidget>

#include <memory>

namespace Akonadi
{
class AgentFilterProxyModel;
class AgentType;
class AgentTypeWidgetPrivate;

/**
 * @short Provides a widget that lists all available agent types.
 *
 * The widget is listening on the dbus for changes, so the
 * widget is updated automatically as soon as new agent types
 * are added to or removed from the system.
 *
 * @code
 *
 * Akonadi::AgentTypeWidget *widget = new Akonadi::AgentTypeWidget( this );
 *
 * // only list agent types that provide contacts
 * widget->agentFilterProxyModel()->addMimeTypeFilter( "text/directory" );
 *
 * @endcode
 *
 * If you want a dialog, you can use the Akonadi::AgentTypeDialog.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADIWIDGETS_EXPORT AgentTypeWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * Creates a new agent type widget.
     *
     * @param parent The parent widget.
     */
    explicit AgentTypeWidget(QWidget *parent = nullptr);

    /**
     * Destroys the agent type widget.
     */
    ~AgentTypeWidget();

    /**
     * Returns the current agent type or an invalid agent type
     * if no agent type is selected.
     */
    Q_REQUIRED_RESULT AgentType currentAgentType() const;

    /**
     * Returns the agent filter proxy model, use this to filter by
     * agent mimetype or capabilities.
     */
    Q_REQUIRED_RESULT AgentFilterProxyModel *agentFilterProxyModel() const;

Q_SIGNALS:
    /**
     * This signal is emitted whenever the current agent type changes.
     *
     * @param current The current agent type.
     * @param previous The previous agent type.
     */
    void currentChanged(const Akonadi::AgentType &current, const Akonadi::AgentType &previous);

    /**
     * This signal is emitted whenever the user activates an agent.
     * @since 4.2
     */
    void activated();

private:
    /// @cond PRIVATE
    std::unique_ptr<AgentTypeWidgetPrivate> const d;
    /// @endcond
};

}

