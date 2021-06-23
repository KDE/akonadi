/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagselectiondialog.h"
#include "controlgui.h"
#include "monitor.h"
#include "tagmodel.h"
#include "ui_tagselectiondialog.h"

#include <KConfigGroup>
#include <KSharedConfig>

using namespace Akonadi;

class Q_DECL_HIDDEN TagSelectionDialog::Private
{
public:
    explicit Private(QDialog *parent)
        : q(parent)
    {
    }

    void writeConfig() const;
    void readConfig() const;

    QDialog *const q;
    Ui::TagSelectionDialog ui;
};

void TagSelectionDialog::Private::writeConfig() const
{
    KConfigGroup group(KSharedConfig::openStateConfig(), "TagSelectionDialog");
    group.writeEntry("Size", q->size());
}

void TagSelectionDialog::Private::readConfig() const
{
    KConfigGroup group(KSharedConfig::openStateConfig(), "TagSelectionDialog");
    const QSize sizeDialog = group.readEntry("Size", QSize(500, 400));
    if (sizeDialog.isValid()) {
        q->resize(sizeDialog);
    }
}

TagSelectionDialog::TagSelectionDialog(QWidget *parent)
    : QDialog(parent)
    , d(new Private(this))
{
    d->ui.setupUi(this);

    auto monitor = new Monitor(this);
    monitor->setObjectName(QStringLiteral("TagSelectionDialogMonitor"));
    monitor->setTypeMonitored(Monitor::Tags);

    d->ui.tagWidget->setModel(new TagModel(monitor, this));
    d->ui.tagWidget->setSelectionEnabled(true);

    d->readConfig();

    ControlGui::widgetNeedsAkonadi(this);
}

TagSelectionDialog::TagSelectionDialog(TagModel *model, QWidget *parent)
    : QDialog(parent)
    , d(new Private(this))
{
    d->ui.setupUi(this);

    d->ui.tagWidget->setModel(model);
    d->ui.tagWidget->setSelectionEnabled(true);

    d->readConfig();

    ControlGui::widgetNeedsAkonadi(this);
}

TagSelectionDialog::~TagSelectionDialog()
{
    d->writeConfig();
}

QDialogButtonBox *TagSelectionDialog::buttons() const
{
    return d->ui.buttonBox;
}

Tag::List TagSelectionDialog::selection() const
{
    return d->ui.tagWidget->selection();
}

void TagSelectionDialog::setSelection(const Tag::List &tags)
{
    d->ui.tagWidget->setSelection(tags);
}
