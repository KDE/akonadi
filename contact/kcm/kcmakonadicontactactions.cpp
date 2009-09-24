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

#include "kcmakonadicontactactions.h"

#include "contactactionssettings.h"
#include "ui_akonadicontactactions.h"

#include <QtGui/QVBoxLayout>

#include <kaboutdata.h>
#include <kcomponentdata.h>
#include <kconfigdialogmanager.h>
#include <kdemacros.h>
#include <kgenericfactory.h>
#include <klocale.h>

K_PLUGIN_FACTORY( KCMAkonadiContactActionsFactory, registerPlugin<KCMAkonadiContactActions>(); )
K_EXPORT_PLUGIN( KCMAkonadiContactActionsFactory( "kcmakonadicontactactions" ) )

KCMAkonadiContactActions::KCMAkonadiContactActions( QWidget *parent, const QVariantList& )
  : KCModule( KCMAkonadiContactActionsFactory::componentData(), parent )
{
  KAboutData *about = new KAboutData( I18N_NOOP( "kcmakonadicontactactions" ), 0,
                                      ki18n( "Contact Actions Settings" ),
                                      0, KLocalizedString(), KAboutData::License_LGPL,
                                      ki18n( "(c) 2009 Tobias Koenig" ) );

  about->addAuthor( ki18n("Tobias Koenig"), KLocalizedString(), "tokoe@kde.org" );

  setAboutData( about );

  QVBoxLayout *layout = new QVBoxLayout( this );
  QWidget *wdg = new QWidget;
  layout->addWidget( wdg );

  Ui_AkonadiContactActions ui;
  ui.setupUi( wdg );

  mConfigManager = addConfig( ContactActionsSettings::self(), wdg );

  load();
}

void KCMAkonadiContactActions::load()
{
  mConfigManager->updateWidgets();
}

void KCMAkonadiContactActions::save()
{
  mConfigManager->updateSettings();
}

void KCMAkonadiContactActions::defaults()
{
  mConfigManager->updateWidgetsDefault();
}

#include "kcmakonadicontactactions.moc"
