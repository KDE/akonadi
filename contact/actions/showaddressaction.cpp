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

#include "showaddressaction.h"

#include "contactactionssettings.h"

#include <kabc/address.h>
#include <krun.h>
#include <ktoolinvocation.h>

using namespace Akonadi;

static void replaceArguments(QString &templateStr, const KABC::Address &address)
{
    templateStr.replace(QStringLiteral("%s"), address.street()).
    replace(QStringLiteral("%r"), address.region()).
    replace(QStringLiteral("%l"), address.locality()).
    replace(QStringLiteral("%z"), address.postalCode()).
    replace(QStringLiteral("%n"), address.country()).
    replace(QStringLiteral("%c"), address.countryToISO(address.country()));
}

void ShowAddressAction::showAddress(const KABC::Address &address)
{
    // synchronize
    ContactActionsSettings::self()->readConfig();

    if (ContactActionsSettings::self()->showAddressAction() == ContactActionsSettings::UseBrowser) {
        QString urlTemplate = ContactActionsSettings::self()->addressUrl();
        replaceArguments(urlTemplate, address);
        if (!urlTemplate.isEmpty()) {
            KToolInvocation::invokeBrowser(urlTemplate);
        }
    } else if (ContactActionsSettings::self()->showAddressAction() == ContactActionsSettings::UseExternalAddressApplication) {
        QString commandTemplate = ContactActionsSettings::self()->addressCommand();
        replaceArguments(commandTemplate, address);

        if (!commandTemplate.isEmpty()) {
            KRun::runCommand(commandTemplate, 0);
        }
    } else if (ContactActionsSettings::self()->showAddressAction() == ContactActionsSettings::UseGooglemap) {
        QString urlTemplate = QStringLiteral("https://maps.google.com/maps?q=%s,%l,%c");
        replaceArguments(urlTemplate, address);
        if (!urlTemplate.isEmpty()) {
            KToolInvocation::invokeBrowser(urlTemplate);
        }
    } else if (ContactActionsSettings::self()->showAddressAction() == ContactActionsSettings::UseMapquest) {
        QString urlTemplate = QStringLiteral("http://open.mapquest.com/?q=%s,%l,%c");
        replaceArguments(urlTemplate, address);
        if (!urlTemplate.isEmpty()) {
            KToolInvocation::invokeBrowser(urlTemplate);
        }
    }
}
