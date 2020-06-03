/*
    This file is part of Akonadi

    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "tagmanagementdialog.h"
#include "ui_tagmanagementdialog.h"

#include "tagmodel.h"
#include "monitor.h"
#include "controlgui.h"

#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>

using namespace Akonadi;

struct Q_DECL_HIDDEN TagManagementDialog::Private {
    explicit Private(QDialog *parent)
        : q(parent)
    {}

    void writeConfig() const;
    void readConfig() const;

    Ui::TagManagementDialog ui;
    QDialog * const q;
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
    Monitor *monitor = new Monitor(this);
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

