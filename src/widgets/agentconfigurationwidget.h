/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"
#include <QDialogButtonBox>
#include <QWidget>

#include <memory>

namespace Akonadi
{
class AgentInstance;
class AgentConfigurationDialog;
class AgentConfigurationWidgetPrivate;

/*!
 * \brief A widget for displaying agent configuration in applications.
 *
 * To implement an agent configuration widget, see AgentConfigurationBase.
 *
 * \class Akonadi::AgentConfigurationWidget
 * \inheaderfile Akonadi/AgentConfigurationWidget
 * \inmodule AkonadiWidgets
 */
class AKONADIWIDGETS_EXPORT AgentConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    /*!
     * Creates a new agent configuration widget for the given agent instance.
     * \a instance The agent instance to configure.
     * \a parent The parent widget.
     */
    explicit AgentConfigurationWidget(const Akonadi::AgentInstance &instance, QWidget *parent = nullptr);
    /*!
     * Destroys the agent configuration widget.
     */
    ~AgentConfigurationWidget() override;

    /*!
     * Loads the agent configuration into the widget.
     */
    void load();
    /*!
     * Saves the configuration from the widget to the agent.
     */
    void save();

Q_SIGNALS:
    /*!
     * Emitted to control the enabled state of the OK button.
     * \a enabled True to enable the OK button, false to disable it.
     */
    void enableOkButton(bool enabled);

protected:
    void childEvent(QChildEvent *event) override;

private:
    friend class AgentConfigurationWidgetPrivate;
    friend class AgentConfigurationDialog;
    std::unique_ptr<AgentConfigurationWidgetPrivate> const d;
};

}
