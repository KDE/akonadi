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

/*!
 * \brief Provides a widget that lists all available agent types.
 *
 * The widget is listening on the dbus for changes, so the
 * widget is updated automatically as soon as new agent types
 * are added to or removed from the system.
 *
 * \code
 *
 * Akonadi::AgentTypeWidget *widget = new Akonadi::AgentTypeWidget( this );
 *
 * // only list agent types that provide contacts
 * widget->agentFilterProxyModel()->addMimeTypeFilter( "text/directory" );
 *
 * \endcode
 *
 * If you want a dialog, you can use the Akonadi::AgentTypeDialog.
 *
 * \class Akonadi::AgentTypeWidget
 * \inheaderfile Akonadi/AgentTypeWidget
 * \inmodule AkonadiWidgets
 *
 * \author Tobias Koenig <tokoe@kde.org>
 */
class AKONADIWIDGETS_EXPORT AgentTypeWidget : public QWidget
{
    Q_OBJECT

public:
    /*!
     * Creates a new agent type widget.
     *
     * \a parent The parent widget.
     */
    explicit AgentTypeWidget(QWidget *parent = nullptr);

    /*!
     * Destroys the agent type widget.
     */
    ~AgentTypeWidget() override;

    /*!
     * Returns the current agent type or an invalid agent type
     * if no agent type is selected.
     */
    [[nodiscard]] AgentType currentAgentType() const;

    /*!
     * Returns the agent filter proxy model, use this to filter by
     * agent mimetype or capabilities.
     */
    [[nodiscard]] AgentFilterProxyModel *agentFilterProxyModel() const;

Q_SIGNALS:
    /*!
     * This signal is emitted whenever the current agent type changes.
     *
     * \a current The current agent type.
     * \a previous The previous agent type.
     */
    void currentChanged(const Akonadi::AgentType &current, const Akonadi::AgentType &previous);

    /*!
     * This signal is emitted whenever the user activates an agent.
     * \since 4.2
     */
    void activated();

private:
    std::unique_ptr<AgentTypeWidgetPrivate> const d;
};

}
