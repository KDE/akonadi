/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_AGENTCONFIGURATIONDIALOG
#define AKONADI_AGENTCONFIGURATIONDIALOG

#include <QDialog>

#include "akonadiwidgets_export.h"

namespace Akonadi
{
class AgentInstance;
class AKONADIWIDGETS_EXPORT AgentConfigurationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AgentConfigurationDialog(const AgentInstance &instance, QWidget *parent = nullptr);
    ~AgentConfigurationDialog() override;

    void accept() override;

private:
    class Private;
    const QScopedPointer<Private> d;
};

}

#endif // AKONADI_AGENTCONFIGURATIONDIALOG
