/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagmanagementdialog.h"
#include "ui_tagmanagementdialog.h"

#include "controlgui.h"
#include "monitor.h"
#include "tagmodel.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

using namespace Akonadi;

struct Q_DECL_HIDDEN TagManagementDialog::Private {
    explicit Private(QDialog *parent)
        : q(parent)
    {
    }

    void writeConfig() const;
    void readConfig() const;

    Ui::TagManagementDialog ui;
    QDialog *const q;
    QDialogButtonBox *buttonBox = nullptr;
};

void TagManagementDialog::Private::writeConfig() const
{
    KConfigGroup group(KSharedConfig::openConfig(), "TagManagementDialog");
    group.writeEntry("Size", q->size());
}

void TagManagementDialog::Private::readConfig() const
{
    KConfigGroup group(KSharedConfig::openConfig(), "TagManagementDialog");
    const QSize sizeDialog = group.readEntry("Size", QSize(500, 400));
    if (sizeDialog.isValid()) {
        q->resize(sizeDialog);
    }
}

TagManagementDialog::TagManagementDialog(QWidget *parent)
    : QDialog(parent)
    , d(new Private(this))
{
    auto monitor = new Monitor(this);
    monitor->setObjectName(QStringLiteral("TagManagementDialogMonitor"));
    monitor->setTypeMonitored(Monitor::Tags);

    d->ui.setupUi(this);
    d->ui.tagEditWidget->setModel(new TagModel(monitor, this));
    d->ui.tagEditWidget->setSelectionEnabled(false);

    d->readConfig();

    ControlGui::widgetNeedsAkonadi(this);
}

TagManagementDialog::~TagManagementDialog()
{
    d->writeConfig();
}

QDialogButtonBox *TagManagementDialog::buttons() const
{
    return d->buttonBox;
}
