/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDialog>

#include "akonadiwidgets_export.h"

#include <memory>

namespace Akonadi
{
class AgentInstance;
class AgentConfigurationDialogPrivate;

/*!
 * \class Akonadi::AgentConfigurationDialog
 * \inheaderfile Akonadi/AgentConfigurationDialog
 * \inmodule AkonadiWidgets
 *
 * \brief The AgentConfigurationDialog class
 */
class AKONADIWIDGETS_EXPORT AgentConfigurationDialog : public QDialog
{
    Q_OBJECT
public:
    /*!
     * Creates a new agent configuration dialog for the given agent instance.
     * \a instance The agent instance to configure.
     * \a parent The parent widget.
     */
    explicit AgentConfigurationDialog(const AgentInstance &instance, QWidget *parent = nullptr);
    /*!
     * Destroys the agent configuration dialog.
     */
    ~AgentConfigurationDialog() override;

    /*!
     * Accepts the dialog and saves the agent configuration.
     */
    void accept() override;

private:
    std::unique_ptr<AgentConfigurationDialogPrivate> const d;
};

}
