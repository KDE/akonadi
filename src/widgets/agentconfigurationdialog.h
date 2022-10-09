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

class AKONADIWIDGETS_EXPORT AgentConfigurationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AgentConfigurationDialog(const AgentInstance &instance, QWidget *parent = nullptr);
    ~AgentConfigurationDialog() override;

    void accept() override;

private:
    std::unique_ptr<AgentConfigurationDialogPrivate> const d;
};

}
