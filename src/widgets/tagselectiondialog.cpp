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
#include "tagmodel.h"
#include "monitor.h"
#include "tageditwidget_p.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

using namespace Akonadi;

struct TagSelectionDialog::Private {
    Private(QDialog *parent)
        : d(parent)
        , mTagWidget(0)
        , mButtonBox(0)
    {
    }
    void writeConfig();
    void readConfig();
    QDialog *d;
    Akonadi::TagEditWidget *mTagWidget;
    QDialogButtonBox *mButtonBox;
};

void TagSelectionDialog::Private::writeConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "TagSelectionDialog");
    group.writeEntry("Size", d->size());
}

void TagSelectionDialog::Private::readConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "TagSelectionDialog");
    const QSize sizeDialog = group.readEntry("Size", QSize(500, 400));
    if (sizeDialog.isValid()) {
        d->resize(sizeDialog);
    }
}

TagSelectionDialog::TagSelectionDialog(QWidget *parent)
    : QDialog(parent)
    , d(new Private(this))
{
    setWindowTitle(i18nc("@title:window", "Manage Tags"));
    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);

    Monitor *monitor = new Monitor(this);
    monitor->setTypeMonitored(Monitor::Tags);

    Akonadi::TagModel *model = new Akonadi::TagModel(monitor, this);
    d->mTagWidget = new Akonadi::TagEditWidget(model, this, true);

    vbox->addWidget(d->mTagWidget);

    d->mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QPushButton *okButton = d->mButtonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(d->mButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(d->mButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

    vbox->addWidget(d->mButtonBox);

    d->readConfig();
}

TagSelectionDialog::~TagSelectionDialog()
{
    d->writeConfig();
}

QDialogButtonBox *TagSelectionDialog::buttons() const
{
    return d->mButtonBox;
}

Tag::List TagSelectionDialog::selection() const
{
    return d->mTagWidget->selection();
}

void TagSelectionDialog::setSelection(const Tag::List &tags)
{
    d->mTagWidget->setSelection(tags);
}
