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

#include "publishdialog_p.h"

#include <akonadi/contact/emailaddressselectiondialog.h>
#include <kpimutils/email.h>
#include <kcalcore/person.h>

#include <KLocalizedString>
#include <QTreeView>

using namespace KCalCore;
using namespace Akonadi;

PublishDialog::Private::Private(PublishDialog *qq) : QObject(), q(qq)
{
}

void PublishDialog::Private::addItem()
{
    mUI.mNameLineEdit->setEnabled(true);
    mUI.mEmailLineEdit->setEnabled(true);
    QListWidgetItem *item = mUI.mListWidget->currentItem();
    if (item) {
        if (!item->text().isEmpty()) {
            item = new QListWidgetItem(mUI.mListWidget);
            mUI.mListWidget->addItem(item);
        }
    } else {
        item = new QListWidgetItem(mUI.mListWidget);
        mUI.mListWidget->addItem(item);
    }
    mUI.mListWidget->setItemSelected(item, true);
    mUI.mNameLineEdit->setClickMessage(i18n("(EmptyName)"));
    mUI.mEmailLineEdit->setClickMessage(i18n("(EmptyEmail)"));
    mUI.mListWidget->setCurrentItem(item);
    mUI.mRemove->setEnabled(true);
}

void PublishDialog::Private::removeItem()
{
    if (mUI.mListWidget->selectedItems().isEmpty()) {
        return;
    }
    QListWidgetItem *item;
    item = mUI.mListWidget->selectedItems().first();

    int row = mUI.mListWidget->row(item);
    mUI.mListWidget->takeItem(row);

    if (!mUI.mListWidget->count()) {
        mUI.mNameLineEdit->setText(QString());
        mUI.mNameLineEdit->setEnabled(false);
        mUI.mEmailLineEdit->setText(QString());
        mUI.mEmailLineEdit->setEnabled(false);
        mUI.mRemove->setEnabled(false);
        return;
    }
    if (row > 0) {
        row--;
    }

    mUI.mListWidget->setCurrentRow(row);
}

void PublishDialog::Private::openAddressbook()
{
    QWeakPointer<Akonadi::EmailAddressSelectionDialog> dialog(
        new Akonadi::EmailAddressSelectionDialog(q));
    dialog.data()->view()->view()->setSelectionMode(QAbstractItemView::ExtendedSelection);

    if (dialog.data()->exec() == QDialog::Accepted) {

        Akonadi::EmailAddressSelectionDialog *dialogPtr = dialog.data();
        if (dialogPtr) {
            const Akonadi::EmailAddressSelection::List selections = dialogPtr->selectedAddresses();
            if (!selections.isEmpty()) {
                foreach(const Akonadi::EmailAddressSelection &selection, selections) {
                    mUI.mNameLineEdit->setEnabled(true);
                    mUI.mEmailLineEdit->setEnabled(true);
                    QListWidgetItem *item = new QListWidgetItem(mUI.mListWidget);
                    mUI.mListWidget->setItemSelected(item, true);
                    mUI.mNameLineEdit->setText(selection.name());
                    mUI.mEmailLineEdit->setText(selection.email());
                    mUI.mListWidget->addItem(item);
                }

                mUI.mRemove->setEnabled(true);
            }
        }
    }
    delete dialog.data();
}

void PublishDialog::Private::updateItem()
{
    if (!mUI.mListWidget->selectedItems().count()) {
        return;
    }

    Person person(mUI.mNameLineEdit->text(), mUI.mEmailLineEdit->text());
    QListWidgetItem *item = mUI.mListWidget->selectedItems().first();
    item->setText(person.fullName());
}

void PublishDialog::Private::updateInput()
{
    if (!mUI.mListWidget->selectedItems().count()) {
        return;
    }

    mUI.mNameLineEdit->setEnabled(true);
    mUI.mEmailLineEdit->setEnabled(true);

    QListWidgetItem *item = mUI.mListWidget->selectedItems().first();
    QString mail, name;
    KPIMUtils::extractEmailAddressAndName(item->text(), mail, name);
    mUI.mNameLineEdit->setText(name);
    mUI.mEmailLineEdit->setText(mail);
}

