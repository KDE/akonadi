/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "collectiongeneralpropertiespage.h"

#include <klocale.h>

using namespace Akonadi;

CollectionGeneralPropertiesPage::CollectionGeneralPropertiesPage(QWidget * parent) :
    CollectionPropertiesPage( parent )
{
  setPageTitle( i18nc( "@title:tab general properties page", "&General" ) );
  ui.setupUi( this );
}

void CollectionGeneralPropertiesPage::load(const Collection & collection)
{
  ui.nameEdit->setText( collection.name() );
  if ( collection.status().count() >= 0 ) {
    ui.countLabel->setText( i18np( "One object", "%1 objects", collection.status().count() ) );
  } else {
    ui.statsBox->hide();
  }
}

void CollectionGeneralPropertiesPage::save(Collection & collection)
{
  collection.setName( ui.nameEdit->text() );
}

#include "collectiongeneralpropertiespage.moc"
