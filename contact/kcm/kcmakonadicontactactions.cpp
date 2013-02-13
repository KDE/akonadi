/*
    This file is part of Akonadi Contact.

    Copyright (c) 2013 Laurent Montel <montel@kde.org>
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

#include "kcmakonadicontactactions.h"

#include "contactactionssettings.h"

#include <QVBoxLayout>

#include <kaboutdata.h>
#include <kcomponentdata.h>
#include <kconfigdialogmanager.h>
#include <kpluginfactory.h>
#include <klocale.h>

Q_DECLARE_METATYPE(ContactActionsSettings::EnumDialPhoneNumberAction)

K_PLUGIN_FACTORY( KCMAkonadiContactActionsFactory, registerPlugin<KCMAkonadiContactActions>(); )
K_EXPORT_PLUGIN( KCMAkonadiContactActionsFactory( "kcm_akonadicontact_actions" ) )

KCMAkonadiContactActions::KCMAkonadiContactActions( QWidget *parent, const QVariantList& )
  : KCModule( KCMAkonadiContactActionsFactory::componentData(), parent )
{
  KAboutData *about = new KAboutData( I18N_NOOP( "kcmakonadicontactactions" ), 0,
                                      ki18n( "Contact Actions Settings" ),
                                      0, KLocalizedString(), KAboutData::License_LGPL,
                                      ki18n( "(c) 2009 Tobias Koenig" ) );

  about->addAuthor( ki18n( "Tobias Koenig" ), KLocalizedString(), "tokoe@kde.org" );

  setAboutData( about );

  QVBoxLayout *layout = new QVBoxLayout( this );
  QWidget *wdg = new QWidget;
  layout->addWidget( wdg );

  ui.setupUi( wdg );

  mConfigManager = addConfig( ContactActionsSettings::self(), wdg );

  ui.DialPhoneNumberAction->addItem(i18n("Skype"), QVariant::fromValue(ContactActionsSettings::UseSkype));
  ui.DialPhoneNumberAction->addItem(i18n("Ekiga"), QVariant::fromValue(ContactActionsSettings::UseEkiga));
  ui.DialPhoneNumberAction->addItem(i18n("SflPhone"), QVariant::fromValue(ContactActionsSettings::UseSflPhone));
#ifdef Q_OS_WINCE
  ui.DialPhoneNumberAction->addItem(i18n("WinCE"), QVariant::fromValue(ContactActionsSettings::UseWinCE));
#endif
  ui.DialPhoneNumberAction->addItem(i18n("External Application"), QVariant::fromValue(ContactActionsSettings::UseExternalPhoneApplication));

  connect(ui.DialPhoneNumberAction, SIGNAL(currentIndexChanged(int)), SLOT(slotDialPhoneNumberActionChanged(int)));
  load();
}

void KCMAkonadiContactActions::slotDialPhoneNumberActionChanged(int value)
{
    ContactActionsSettings::EnumDialPhoneNumberAction enumValue = ui.DialPhoneNumberAction->itemData(value).value<ContactActionsSettings::EnumDialPhoneNumberAction>();
    if (enumValue == ContactActionsSettings::UseExternalPhoneApplication) {
        ui.stackedWidget_2->setCurrentIndex(1);
    } else {
        ui.stackedWidget_2->setCurrentIndex(0);
    }
    emit changed(true);
}

void KCMAkonadiContactActions::load()
{
  mConfigManager->updateWidgets();
  ui.DialPhoneNumberAction->setCurrentIndex(ContactActionsSettings::self()->dialPhoneNumberAction());
}

void KCMAkonadiContactActions::save()
{
  ContactActionsSettings::self()->setDialPhoneNumberAction(ui.DialPhoneNumberAction->currentIndex());
  mConfigManager->updateSettings();
}

void KCMAkonadiContactActions::defaults()
{
  mConfigManager->updateWidgetsDefault();
  const bool bUseDefaults = ContactActionsSettings::self()->useDefaults( true );
  ui.DialPhoneNumberAction->setCurrentIndex(ContactActionsSettings::self()->dialPhoneNumberAction());
  ContactActionsSettings::self()->useDefaults( bUseDefaults );
}

