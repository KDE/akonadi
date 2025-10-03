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
    setWindowTitle(i18nc("@title:window, %1 = agent name", "%1 Configuration", instance.name()));
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
        connect(d->widget.data(), &AgentConfigurationWidget::enableOkButton, applyButton, &QPushButton::setEnabled);
    }
    if ((d->okButton = btnBox->button(QDialogButtonBox::Ok))) {
        connect(d->widget.data(), &AgentConfigurationWidget::enableOkButton, d->okButton, &QPushButton::setEnabled);
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

#include "moc_agentconfigurationdialog.cpp"
