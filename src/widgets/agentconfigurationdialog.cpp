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
#include "core/agentmanager.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

#include <KLocalizedString>

namespace Akonadi {
class Q_DECL_HIDDEN AgentConfigurationDialog::Private
{
public:
    AgentConfigurationWidget *widget = nullptr;
};
}

using namespace Akonadi;

AgentConfigurationDialog::AgentConfigurationDialog(const AgentInstance &instance, QWidget *parent)
    : QDialog(parent)
    , d(new Private)
{
    setWindowTitle(i18nc("%1 = agent name", "%1 Configuration", instance.name()));
    setWindowIcon(instance.type().icon());

    auto l = new QVBoxLayout;
    setLayout(l);

    d->widget = new AgentConfigurationWidget(instance, this);
    l->addWidget(d->widget);

    auto btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
    l->addWidget(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, this, &AgentConfigurationDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &AgentConfigurationDialog::reject);
    connect(btnBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            d->widget, &AgentConfigurationWidget::save);
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

