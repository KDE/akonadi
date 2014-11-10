/*
  Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2009 Allen Winter <winter@kde.org>

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

// TODO: validate hand-entered email addresses
// TODO: don't allow duplicates; at least remove dupes when passing back
// TODO: the list in PublishDialog::addresses()

#include "publishdialog_p.h"

#include <kcalcore/attendee.h>
#include <kcalcore/person.h>

#include <KLocalizedString>

using namespace KCalCore;
using namespace Akonadi;

PublishDialog::PublishDialog(QWidget *parent)
    : KDialog(parent), d(new Private(this))
{
    setCaption(i18n("Select Addresses"));
    setButtons(Ok|Cancel|Help);
    setHelp("group-scheduling", "korganizer");
    QWidget *widget = new QWidget(this);
    widget->setObjectName("PublishFreeBusy");
    d->mUI.setupUi(widget);
    setMainWidget(widget);
    d->mUI.mListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    d->mUI.mNameLineEdit->setEnabled(false);
    d->mUI.mEmailLineEdit->setEnabled(false);

    setButtonToolTip(Ok, i18n("Send email to these recipients"));
    setButtonWhatsThis(Ok, i18n("Clicking the <b>Ok</b> button will cause "
                                "an email to be sent to the recipients you "
                                "have entered."));
    setButtonToolTip(Cancel, i18n("Cancel recipient selection and the email"));
    setButtonWhatsThis(Cancel, i18n("Clicking the <b>Cancel</b> button will "
                                    "cause the email operation to be terminated."));

    setButtonWhatsThis(Help, i18n("Click the <b>Help</b> button to read "
                                  "more information about Group Scheduling."));

    d->mUI.mNew->setIcon(KIcon("list-add"));
    d->mUI.mRemove->setIcon(KIcon("list-remove"));
    d->mUI.mRemove->setEnabled(false);
    d->mUI.mSelectAddressee->setIcon(KIcon("view-pim-contacts"));
    d->mUI.mNameLineEdit->setTrapReturnKey( true );
    d->mUI.mEmailLineEdit->setTrapReturnKey( true );    
    connect(d->mUI.mListWidget, SIGNAL(itemSelectionChanged()),
            d, SLOT(updateInput()));
    connect(d->mUI.mNew, SIGNAL(clicked()),
            d, SLOT(addItem()));
    connect(d->mUI.mRemove, SIGNAL(clicked()),
            d, SLOT(removeItem()));
    connect(d->mUI.mSelectAddressee, SIGNAL(clicked()),
            d, SLOT(openAddressbook()));
    connect(d->mUI.mNameLineEdit, SIGNAL(textChanged(QString)),
            d, SLOT(updateItem()));
    connect(d->mUI.mEmailLineEdit, SIGNAL(textChanged(QString)),
            d, SLOT(updateItem()));
}

PublishDialog::~PublishDialog()
{
    delete d;
}

void PublishDialog::addAttendee(const Attendee::Ptr &attendee)
{
    d->mUI.mNameLineEdit->setEnabled(true);
    d->mUI.mEmailLineEdit->setEnabled(true);
    QListWidgetItem *item = new QListWidgetItem(d->mUI.mListWidget);
    Person person(attendee->name(), attendee->email());
    item->setText(person.fullName());
    d->mUI.mListWidget->addItem(item);
    d->mUI.mRemove->setEnabled(!d->mUI.mListWidget->selectedItems().isEmpty());
}

QString PublishDialog::addresses() const
{
    QString to;
    QListWidgetItem *item;
    const int count = d->mUI.mListWidget->count();
    for (int i=0; i<count; ++i) {
        item = d->mUI.mListWidget->item(i);
        if (!item->text().isEmpty()) {
            to += item->text();
            if (i < count-1) {
                to += ", ";
            }
        }
    }
    return to;
}

