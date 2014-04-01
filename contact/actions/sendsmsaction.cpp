/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 Felix Mauch (felix_mauch@web.de)

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

#include "sendsmsaction.h"

#include "contactactionssettings.h"
#include "qskypedialer.h"
#include "smsdialog.h"

#include <kabc/phonenumber.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <krun.h>

#include <QPointer>

static QString strippedSmsNumber(const QString &number)
{
    QString result;

    for (int i = 0; i < number.length(); ++i) {
        const QChar character = number.at(i);
        if (character.isDigit() || (character == QLatin1Char('+') && i == 0)) {
            result += character;
        }
    }

    return result;
}

void SendSmsAction::sendSms(const KABC::PhoneNumber &phoneNumber)
{
    const QString number = phoneNumber.number().trimmed();

    // synchronize
    ContactActionsSettings::self()->readConfig();

    QString command = ContactActionsSettings::self()->smsCommand();

    if (command.isEmpty()) {
        KMessageBox::sorry(0, i18n("There is no application set which could be executed.\nPlease go to the settings dialog and configure one."));
        return;
    }

    QPointer<SmsDialog> dlg(new SmsDialog(number));
    if (dlg->exec() != QDialog::Accepted) {   // the cancel button has been clicked
        delete dlg;
        return;
    }
    const QString message = (dlg != 0 ? dlg->message() : QString());
    delete dlg;

    //   we handle skype separated
    if (ContactActionsSettings::self()->sendSmsAction() == ContactActionsSettings::UseSkypeSms) {
        QSkypeDialer dialer(QStringLiteral("AkonadiContacts"));
        if (dialer.sendSms(number, message)) {
            // I'm not sure whether here should be a notification.
            // Skype can do a notification itself if whished.
        } else {
            KMessageBox::sorry(0, dialer.errorMessage());
        }

        return;
    }

    /*
     * %N the raw number
     * %n the number with all additional non-number characters removed
     */
    command = command.replace(QStringLiteral("%N"), phoneNumber.number());
    command = command.replace(QStringLiteral("%n"), strippedSmsNumber(number));
    command = command.replace(QStringLiteral("%t"), message);
    //Bug: 293232 In KDE3 We used %F to replace text
    command = command.replace(QStringLiteral("%F"), message);
    KRun::runCommand(command, 0);
}
