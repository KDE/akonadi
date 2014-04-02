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

#include "imeditwidget.h"
#include "customfieldseditwidget.h"

#include "im/imeditordialog.h"
#include "im/improtocols.h"

#include <QtCore/QPointer>
#include <QHBoxLayout>
#include <QToolButton>

#include <kabc/addressee.h>
#include <klineedit.h>
#include <klocalizedstring.h>

IMEditWidget::IMEditWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);

    mIMEdit = new QLineEdit;
    layout->addWidget(mIMEdit);

    mEditButton = new QToolButton;
    mEditButton->setText(i18n("..."));
    layout->addWidget(mEditButton);
    setFocusProxy(mEditButton);
    setFocusPolicy(Qt::StrongFocus);

    connect(mEditButton, SIGNAL(clicked()), SLOT(edit()));
}

IMEditWidget::~IMEditWidget()
{
}

void IMEditWidget::loadContact(const KABC::Addressee &contact)
{
    mIMEdit->setText(contact.custom(QStringLiteral("KADDRESSBOOK"), QStringLiteral("X-IMAddress")));

    const QStringList customs = contact.customs();

    foreach (const QString &custom, customs) {
        QString app, name, value;
        splitCustomField(custom, app, name, value);

        if (app.startsWith(QStringLiteral("messaging/"))) {
            if (name == QLatin1String("All")) {
                const QString protocol = app;
                const QStringList names = value.split(QChar(0xE000), QString::SkipEmptyParts);

                foreach (const QString &name, names) {
                    mIMAddresses << IMAddress(protocol, name, (name == mIMEdit->text()));
                }
            }
        }
    }
}

void IMEditWidget::storeContact(KABC::Addressee &contact) const
{
    if (!mIMEdit->text().isEmpty()) {
        contact.insertCustom(QStringLiteral("KADDRESSBOOK"), QStringLiteral("X-IMAddress"), mIMEdit->text());
    } else {
        contact.removeCustom(QStringLiteral("KADDRESSBOOK"), QStringLiteral("X-IMAddress"));
    }

    // create a map with protocol as key and list of names for that protocol as value
    QMap<QString, QStringList> protocolMap;

    // fill map with all known protocols
    foreach (const QString &protocol, IMProtocols::self()->protocols()) {
        protocolMap.insert(protocol, QStringList());
    }

    // add the configured addresses
    foreach (const IMAddress &address, mIMAddresses) {
        protocolMap[address.protocol()].append(address.name());
    }

    // iterate over this list and modify the contact according
    QMapIterator<QString, QStringList> it(protocolMap);
    while (it.hasNext()) {
        it.next();

        if (!it.value().isEmpty()) {
            contact.insertCustom(it.key(), QStringLiteral("All"), it.value().join(QString(0xE000)));
        } else {
            contact.removeCustom(it.key(), QStringLiteral("All"));
        }
    }
}

void IMEditWidget::setReadOnly(bool readOnly)
{
    mIMEdit->setReadOnly(readOnly);
    mEditButton->setEnabled(!readOnly);
}

void IMEditWidget::edit()
{
    QPointer<IMEditorDialog> dlg = new IMEditorDialog(this);
    dlg->setAddresses(mIMAddresses);

    if (dlg->exec() == QDialog::Accepted) {
        mIMAddresses = dlg->addresses();

        foreach (const IMAddress &address, mIMAddresses) {
            if (address.preferred()) {
                mIMEdit->setText(address.name());
                break;
            }
        }
    }

    delete dlg;
}
