/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "nameeditdialog.h"

#include <QFormLayout>

#include <kcombobox.h>
#include <klineedit.h>
#include <klocalizedstring.h>

NameEditDialog::NameEditDialog(QWidget *parent)
    : KDialog(parent)
{
    setCaption(i18n("Edit Contact Name"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);

    QWidget *page = new QWidget(this);
    setMainWidget(page);

    QFormLayout *layout = new QFormLayout(page);

    mPrefixCombo = new KComboBox(page);
    mPrefixCombo->setDuplicatesEnabled(false);
    mPrefixCombo->setEditable(true);

    mGivenNameEdit = new KLineEdit(page);
    mGivenNameEdit->setTrapReturnKey(true);

    mAdditionalNameEdit = new KLineEdit(page);
    mAdditionalNameEdit->setTrapReturnKey(true);

    mFamilyNameEdit = new KLineEdit(page);
    mFamilyNameEdit->setTrapReturnKey(true);

    mSuffixCombo = new KComboBox(page);
    mSuffixCombo->setDuplicatesEnabled(false);
    mSuffixCombo->setEditable(true);

    layout->addRow(i18n("Honorific prefixes:"), mPrefixCombo);
    layout->addRow(i18n("Given name:"), mGivenNameEdit);
    layout->addRow(i18n("Additional names:"), mAdditionalNameEdit);
    layout->addRow(i18n("Family names:"), mFamilyNameEdit);
    layout->addRow(i18n("Honorific suffixes:"), mSuffixCombo);

    QStringList prefixList;
    prefixList += QString();
    prefixList += i18n("Dr.");
    prefixList += i18n("Miss");
    prefixList += i18n("Mr.");
    prefixList += i18n("Mrs.");
    prefixList += i18n("Ms.");
    prefixList += i18n("Prof.");
    prefixList.sort();

    QStringList suffixList;
    suffixList += QString();
    suffixList += i18n("I");
    suffixList += i18n("II");
    suffixList += i18n("III");
    suffixList += i18n("Jr.");
    suffixList += i18n("Sr.");
    suffixList.sort();

    mPrefixCombo->addItems(prefixList);
    mSuffixCombo->addItems(suffixList);

    mPrefixCombo->lineEdit()->setFocus();
}

void NameEditDialog::setFamilyName(const QString &name)
{
    mFamilyNameEdit->setText(name);
}

QString NameEditDialog::familyName() const
{
    return mFamilyNameEdit->text();
}

void NameEditDialog::setGivenName(const QString &name)
{
    mGivenNameEdit->setText(name);
}

QString NameEditDialog::givenName() const
{
    return mGivenNameEdit->text();
}

void NameEditDialog::setPrefix(const QString &prefix)
{
    mPrefixCombo->setItemText(mPrefixCombo->currentIndex(), prefix);
}

QString NameEditDialog::prefix() const
{
    return mPrefixCombo->currentText();
}

void NameEditDialog::setSuffix(const QString &suffix)
{
    mSuffixCombo->setItemText(mSuffixCombo->currentIndex(), suffix);
}

QString NameEditDialog::suffix() const
{
    return mSuffixCombo->currentText();
}

void NameEditDialog::setAdditionalName(const QString &name)
{
    mAdditionalNameEdit->setText(name);
}

QString NameEditDialog::additionalName() const
{
    return mAdditionalNameEdit->text();
}
