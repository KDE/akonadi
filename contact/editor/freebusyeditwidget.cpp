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

#include "freebusyeditwidget.h"

#include <QHBoxLayout>

#include <kabc/addressee.h>
#include <kcalcore/freebusyurlstore.h>
#include <kurlrequester.h>
#include <KLineEdit>

FreeBusyEditWidget::FreeBusyEditWidget(QWidget *parent)
    : QWidget(parent)
    , mReadOnly(false)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);

    mURL = new KUrlRequester;
    mURL->lineEdit()->setTrapReturnKey(true);
    layout->addWidget(mURL);
    setFocusProxy(mURL);
    setFocusPolicy(Qt::StrongFocus);
}

FreeBusyEditWidget::~FreeBusyEditWidget()
{
}

void FreeBusyEditWidget::loadContact(const KABC::Addressee &contact)
{
    if (contact.preferredEmail().isEmpty()) {
        return;
    }

    mURL->setUrl(QUrl(KCalCore::FreeBusyUrlStore::self()->readUrl(contact.preferredEmail())));
}

void FreeBusyEditWidget::storeContact(KABC::Addressee &contact) const
{
    if (contact.preferredEmail().isEmpty()) {
        return;
    }

    KCalCore::FreeBusyUrlStore::self()->writeUrl(contact.preferredEmail(), mURL->url().url());
    KCalCore::FreeBusyUrlStore::self()->sync();
}

void FreeBusyEditWidget::setReadOnly(bool readOnly)
{
    mURL->setEnabled(!readOnly);
}
