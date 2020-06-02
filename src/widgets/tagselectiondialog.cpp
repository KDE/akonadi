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

#include "tagselectiondialog.h"
#include "ui_tagselectiondialog.h"
#include "tagmodel.h"
#include "monitor.h"
#include "controlgui.h"

#include <KConfigGroup>
#include <KSharedConfig>

using namespace Akonadi;

class Q_DECL_HIDDEN TagSelectionDialog::Private
{
public:
    explicit Private(QDialog *parent)
        : q(parent)
    {}

    void writeConfig() const;
    void readConfig() const;

    QDialog * const q = nullptr;
    Ui::TagSelectionDialog ui;
};

void TagSelectionDialog::Private::writeConfig() const
{
    KConfigGroup group(KSharedConfig::openConfig(), "TagSelectionDialog");
    group.writeEntry("Size", q->size());
}

void TagSelectionDialog::Private::readConfig() const
{
    KConfigGroup group(KSharedConfig::openConfig(), "TagSelectionDialog");
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

    Monitor *monitor = new Monitor(this);
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

