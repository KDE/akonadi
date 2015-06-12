/*
IM address item editor widget for KDE PIM

Copyright 2012 Jonathan Marten <jjm@keelhaul.me.uk>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "imitemdialog.h"

#include "immodel.h"
#include "improtocols.h"

#include <QFormLayout>

#include <kcombobox.h>
#include <klineedit.h>
#include <klocalizedstring.h>
#include <kicon.h>

IMItemDialog::IMItemDialog(QWidget *parent)
    : KDialog(parent)
{
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);

    QWidget *widget = new QWidget(this);
    setMainWidget(widget);

    QFormLayout *layout = new QFormLayout(widget);

    mProtocolCombo = new KComboBox;
    mProtocolCombo->addItem(i18nc("@item:inlistbox select from a list of IM protocols",
                                  "Select..."));
    layout->addRow(i18nc("@label:listbox", "Protocol:"), mProtocolCombo);

    const QStringList protocols = IMProtocols::self()->protocols();
    foreach (const QString &protocol, protocols) {
        mProtocolCombo->addItem(KIcon(IMProtocols::self()->icon(protocol)),
                                IMProtocols::self()->name(protocol),
                                protocol);
    }

    mNameEdit = new KLineEdit;
    layout->addRow(i18nc("@label:textbox IM address", "Address:"), mNameEdit);

    connect(mProtocolCombo, SIGNAL(currentIndexChanged(int)), SLOT(slotUpdateButtons()));
    connect(mNameEdit, SIGNAL(textChanged(QString)), SLOT(slotUpdateButtons()));

    slotUpdateButtons();
}

void IMItemDialog::setAddress(const IMAddress &address)
{
    mProtocolCombo->setCurrentIndex(
        IMProtocols::self()->protocols().indexOf(address.protocol()) + 1);

    mNameEdit->setText(address.name());
    slotUpdateButtons();
}

IMAddress IMItemDialog::address() const
{
    return IMAddress(mProtocolCombo->itemData(mProtocolCombo->currentIndex()).toString(),
                     mNameEdit->text(), false);
}

void IMItemDialog::slotUpdateButtons()
{
    enableButtonOk(mProtocolCombo->currentIndex() > 0 && !mNameEdit->text().trimmed().isEmpty());
}
