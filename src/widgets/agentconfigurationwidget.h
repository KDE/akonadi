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
/**
 * @brief A widget for displaying agent configuration in applications.
 *
 * To implement an agent configuration widget, see AgentConfigurationBase.
 */
class AKONADIWIDGETS_EXPORT AgentConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AgentConfigurationWidget(const Akonadi::AgentInstance &instance, QWidget *parent = nullptr);
    ~AgentConfigurationWidget() override;

    void load();
    void save();
    QSize restoreDialogSize() const;
    void saveDialogSize(QSize size);
    QDialogButtonBox::StandardButtons standardButtons() const;

Q_SIGNALS:
    void enableOkButton(bool enabled);

protected:
    void childEvent(QChildEvent *event) override;

private:
    class AgentConfigurationWidgetPrivate;
    friend class AgentConfigurationWidgetPrivate;
    friend class AgentConfigurationDialog;
    std::unique_ptr<AgentConfigurationWidgetPrivate> const d;
};

}

