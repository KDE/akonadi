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
#include "tagmodel.h"
#include "monitor.h"
#include "tageditwidget_p.h"
#include <KLocalizedString>
#include <KSharedConfig>

using namespace Akonadi;

struct TagManagementDialog::Private {
    Private(KDialog *parent)
        : d(parent)
    {
    }
    void writeConfig();
    void readConfig();
    KDialog *d;
};

void TagManagementDialog::Private::writeConfig()
{
    KConfigGroup group(KGlobal::config(), "TagManagementDialog");
    group.writeEntry("Size", d->size());
}

void TagManagementDialog::Private::readConfig()
{
    KConfigGroup group(KGlobal::config(), "TagManagementDialog");
    const QSize sizeDialog = group.readEntry("Size", QSize(500, 400));
    if (sizeDialog.isValid()) {
        d->resize(sizeDialog);
    }
}

TagManagementDialog::TagManagementDialog(QWidget *parent)
    : KDialog(parent)
    , d(new Private(this))
{
    setCaption(i18nc("@title:window", "Manage Tags"));
    setButtons(KDialog::Ok | KDialog::Cancel);
    setDefaultButton(KDialog::Ok);

    Monitor *monitor = new Monitor(this);
    monitor->setTypeMonitored(Monitor::Tags);

    Akonadi::TagModel *model = new Akonadi::TagModel(monitor, this);
    setMainWidget(new Akonadi::TagEditWidget(model, this, false));

    d->readConfig();
}

TagManagementDialog::~TagManagementDialog()
{
    d->writeConfig();
}
