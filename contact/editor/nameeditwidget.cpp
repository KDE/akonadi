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

#include "nameeditwidget.h"

#include "nameeditdialog.h"

#include <QtCore/QPointer>
#include <QHBoxLayout>
#include <QToolButton>

#include <kabc/addressee.h>
#include <kdialog.h>
#include <klineedit.h>
#include <klocalizedstring.h>

NameEditWidget::NameEditWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(KDialog::spacingHint());

    mNameEdit = new KLineEdit;
    mNameEdit->setTrapReturnKey(true);
    layout->addWidget(mNameEdit);
    setFocusProxy(mNameEdit);
    setFocusPolicy(Qt::StrongFocus);

    mButtonEdit = new QToolButton;
    mButtonEdit->setText(i18n("..."));
    layout->addWidget(mButtonEdit);


    connect( mNameEdit, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)) );
    connect( mButtonEdit, SIGNAL(clicked()), this, SLOT(openNameEditDialog()) );
}

NameEditWidget::~NameEditWidget()
{
}

void NameEditWidget::setReadOnly(bool readOnly)
{
    mNameEdit->setReadOnly(readOnly);
    mButtonEdit->setEnabled(!readOnly);
}

void NameEditWidget::loadContact(const KABC::Addressee &contact)
{
    mContact = contact;

    disconnect(mNameEdit, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
    mNameEdit->setText(contact.assembledName());
    connect(mNameEdit, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
}

void NameEditWidget::storeContact(KABC::Addressee &contact) const
{
    contact.setPrefix(mContact.prefix());
    contact.setGivenName(mContact.givenName());
    contact.setAdditionalName(mContact.additionalName());
    contact.setFamilyName(mContact.familyName());
    contact.setSuffix(mContact.suffix());
}

void NameEditWidget::textChanged(const QString &text)
{
    mContact.setNameFromString(text);

    emit nameChanged(mContact);
}

void NameEditWidget::openNameEditDialog()
{
    QPointer<NameEditDialog> dlg = new NameEditDialog(this);

    dlg->setPrefix(mContact.prefix());
    dlg->setGivenName(mContact.givenName());
    dlg->setAdditionalName(mContact.additionalName());
    dlg->setFamilyName(mContact.familyName());
    dlg->setSuffix(mContact.suffix());

    if (dlg->exec() == QDialog::Accepted) {
        mContact.setPrefix(dlg->prefix());
        mContact.setGivenName(dlg->givenName());
        mContact.setAdditionalName(dlg->additionalName());
        mContact.setFamilyName(dlg->familyName());
        mContact.setSuffix(dlg->suffix());

        disconnect(mNameEdit, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
        mNameEdit->setText(mContact.assembledName());
        connect(mNameEdit, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));

        emit nameChanged(mContact);
    }

    delete dlg;
}
