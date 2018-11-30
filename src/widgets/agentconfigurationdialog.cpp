/*
    Copyright (c) 2018 Daniel Vrátil <dvratil@kde.org>

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

#include "agentconfigurationdialog.h"
#include "agentconfigurationwidget.h"
#include "agentconfigurationwidget_p.h"
#include "agentconfigurationbase.h"
#include "core/agentmanager.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QAction>

#include <KLocalizedString>
#include <KHelpMenu>
#include <KAboutData>

namespace Akonadi {
class Q_DECL_HIDDEN AgentConfigurationDialog::Private
{
public:
    QScopedPointer<AgentConfigurationWidget> widget;
};
}

using namespace Akonadi;

AgentConfigurationDialog::AgentConfigurationDialog(const AgentInstance &instance, QWidget *parent)
    : QDialog(parent)
    , d(new Private)
{
    setWindowTitle(i18nc("%1 = agent name", "%1 Configuration", instance.name()));
    setWindowIcon(instance.type().icon());

    auto l = new QVBoxLayout(this);

    d->widget.reset(new AgentConfigurationWidget(instance, this));
    l->addWidget(d->widget.data());

    auto btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
    l->addWidget(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, this, &AgentConfigurationDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &AgentConfigurationDialog::reject);
    connect(btnBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            d->widget.data(), &AgentConfigurationWidget::save);

    if (auto plugin = d->widget->d->plugin) {
        if (auto aboutData = plugin->aboutData()) {
            KHelpMenu *helpMenu = new KHelpMenu(this, *aboutData, true);
            helpMenu->action(KHelpMenu::menuDonate);
            //Initialize menu
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
}

AgentConfigurationDialog::~AgentConfigurationDialog()
{
}

void AgentConfigurationDialog::accept()
{
    if (d->widget) {
        d->widget->save();
    }

    QDialog::accept();
}

