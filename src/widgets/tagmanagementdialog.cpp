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
namespace
{
static const char myTagManagementDialogGroupName[] = "TagManagementDialog";
}
class Akonadi::TagManagementDialogPrivate
{
public:
    explicit TagManagementDialogPrivate(QDialog *parent)
        : q(parent)
    {
    }

    void writeConfig() const;
    void readConfig() const;

    Ui::TagManagementDialog ui;
    QDialog *const q;
    QDialogButtonBox *buttonBox = nullptr;
};

void TagManagementDialogPrivate::writeConfig() const
{
    KConfigGroup group(KSharedConfig::openStateConfig(), QLatin1StringView(myTagManagementDialogGroupName));
    group.writeEntry("Size", q->size());
}

void TagManagementDialogPrivate::readConfig() const
{
    KConfigGroup group(KSharedConfig::openStateConfig(), QLatin1StringView(myTagManagementDialogGroupName));
    const QSize sizeDialog = group.readEntry("Size", QSize(500, 400));
    if (sizeDialog.isValid()) {
        q->resize(sizeDialog);
    }
}

TagManagementDialog::TagManagementDialog(QWidget *parent)
    : QDialog(parent)
    , d(new TagManagementDialogPrivate(this))
{
    auto monitor = new Monitor(this);
    monitor->setObjectName(QLatin1StringView("TagManagementDialogMonitor"));
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

#include "moc_tagmanagementdialog.cpp"
