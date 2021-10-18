/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentconfigurationdialog.h"
#include "agentconfigurationbase.h"
#include "agentconfigurationwidget.h"
#include "agentconfigurationwidget_p.h"
#include "core/agentmanager.h"

#include <QAction>
#include <QDebug>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <KAboutData>
#include <KHelpMenu>
#include <KLocalizedString>

namespace Akonadi
{
class AgentConfigurationDialogPrivate
{
public:
    explicit AgentConfigurationDialogPrivate(AgentConfigurationDialog *qq)
        : q(qq)
    {
    }
    void restoreDialogSize();
    AgentConfigurationDialog *const q;
    QPushButton *okButton = nullptr;
    QScopedPointer<AgentConfigurationWidget> widget;
};

void AgentConfigurationDialogPrivate::restoreDialogSize()
{
    if (widget) {
        const QSize size = widget->restoreDialogSize();
        if (size.isValid()) {
            q->resize(size);
        }
    }
}

} // namespace Akonadi

using namespace Akonadi;

AgentConfigurationDialog::AgentConfigurationDialog(const AgentInstance &instance, QWidget *parent)
    : QDialog(parent)
    , d(new AgentConfigurationDialogPrivate(this))
{
    setWindowTitle(i18nc("%1 = agent name", "%1 Configuration", instance.name()));
    setWindowIcon(instance.type().icon());

    auto l = new QVBoxLayout(this);

    d->widget.reset(new AgentConfigurationWidget(instance, this));
    l->addWidget(d->widget.data());

    auto btnBox = new QDialogButtonBox(d->widget->standardButtons(), this);
    l->addWidget(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, this, &AgentConfigurationDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &AgentConfigurationDialog::reject);
    if (QPushButton *applyButton = btnBox->button(QDialogButtonBox::Apply)) {
        connect(applyButton, &QPushButton::clicked, d->widget.data(), &AgentConfigurationWidget::save);
    }
    if ((d->okButton = btnBox->button(QDialogButtonBox::Ok))) {
        connect(d->widget.data(), &AgentConfigurationWidget::enableOkButton, d->okButton, &QPushButton::setEnabled);
    }

    if (auto plugin = d->widget->d->plugin) {
        if (auto aboutData = plugin->aboutData()) {
            auto helpMenu = new KHelpMenu(this, *aboutData, true);
            helpMenu->action(KHelpMenu::menuDonate);
            // Initialize menu
            QMenu *menu = helpMenu->menu();
            // HACK: the actions are populated from QGuiApplication so they would refer to the
            // current application not to the agent, so we have to adjust the strings in some
            // of the actions.
            helpMenu->action(KHelpMenu::menuAboutApp)->setIcon(QIcon::fromTheme(aboutData->programIconName()));
            helpMenu->action(KHelpMenu::menuHelpContents)->setText(i18n("%1 Handbook", aboutData->displayName()));
            helpMenu->action(KHelpMenu::menuAboutApp)->setText(i18n("About %1", aboutData->displayName()));
            btnBox->addButton(QDialogButtonBox::Help)->setMenu(menu);
        }
    }
    d->restoreDialogSize();
}

AgentConfigurationDialog::~AgentConfigurationDialog()
{
    if (d->widget) {
        d->widget->saveDialogSize(size());
    }
}

void AgentConfigurationDialog::accept()
{
    if (d->widget) {
        d->widget->save();
    }

    QDialog::accept();
}
