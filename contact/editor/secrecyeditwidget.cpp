/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "secrecyeditwidget.h"

#include <QVBoxLayout>

#include <kabc/addressee.h>
#include <kabc/secrecy.h>
#include <kcombobox.h>

SecrecyEditWidget::SecrecyEditWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);

    mSecrecyCombo = new KComboBox(this);
    layout->addWidget(mSecrecyCombo);

    const KABC::Secrecy::TypeList list = KABC::Secrecy::typeList();
    KABC::Secrecy::TypeList::ConstIterator it;

    // (*it) is the type enum, which is also used as the index in the combo
    KABC::Secrecy::TypeList::ConstIterator end(list.constEnd());
    for (it = list.constBegin(); it != end; ++it) {
        mSecrecyCombo->insertItem(*it, KABC::Secrecy::typeLabel(*it));
    }
}

SecrecyEditWidget::~SecrecyEditWidget()
{
}

void SecrecyEditWidget::setReadOnly(bool readOnly)
{
    mSecrecyCombo->setEnabled(!readOnly);
}

void SecrecyEditWidget::loadContact(const KABC::Addressee &contact)
{
    if (contact.secrecy().type() != KABC::Secrecy::Invalid) {
        mSecrecyCombo->setCurrentIndex(contact.secrecy().type());
    }
}

void SecrecyEditWidget::storeContact(KABC::Addressee &contact) const
{
    KABC::Secrecy secrecy;
    secrecy.setType((KABC::Secrecy::Type)mSecrecyCombo->currentIndex());

    contact.setSecrecy(secrecy);
}
