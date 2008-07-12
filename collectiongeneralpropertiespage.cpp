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

#include "collection.h"
#include "collectiondisplayattribute.h"
#include "collectionstatistics.h"
#include "collectionutils_p.h"

#include <klocale.h>

using namespace Akonadi;

CollectionGeneralPropertiesPage::CollectionGeneralPropertiesPage(QWidget * parent) :
    CollectionPropertiesPage( parent )
{
  setPageTitle( i18nc( "@title:tab general properties page", "General" ) );
  ui.setupUi( this );
}

void CollectionGeneralPropertiesPage::load(const Collection & collection)
{
  QString displayName;
  QString iconName;
  if ( collection.hasAttribute<CollectionDisplayAttribute>() ) {
    displayName = collection.attribute<CollectionDisplayAttribute>()->displayName();
    iconName = collection.attribute<CollectionDisplayAttribute>()->iconName();
  }

  if ( displayName.isEmpty() )
    ui.nameEdit->setText( collection.name() );
  else
    ui.nameEdit->setText( displayName );

  if ( iconName.isEmpty() )
    ui.customIcon->setIcon( CollectionUtils::defaultIconName( collection ) );
  else
    ui.customIcon->setIcon( iconName );
  ui.customIconCheckbox->setChecked( !iconName.isEmpty() );

  if ( collection.statistics().count() >= 0 ) {
    ui.countLabel->setText( i18ncp( "@label", "One object", "%1 objects",
                            collection.statistics().count() ) );
  } else {
    ui.statsBox->hide();
  }
}

void CollectionGeneralPropertiesPage::save(Collection & collection)
{
  if ( collection.hasAttribute<CollectionDisplayAttribute>() &&
       !collection.attribute<CollectionDisplayAttribute>()->displayName().isEmpty() )
    collection.attribute<CollectionDisplayAttribute>()->setDisplayName( ui.nameEdit->text() );
  else
    collection.setName( ui.nameEdit->text() );

  if ( ui.customIconCheckbox->isChecked() )
    collection.attribute<CollectionDisplayAttribute>( Collection::AddIfMissing )->setIconName( ui.customIcon->icon() );
  else if ( collection.hasAttribute<CollectionDisplayAttribute>() )
    collection.attribute<CollectionDisplayAttribute>()->setIconName( QString() );
}

#include "collectiongeneralpropertiespage.moc"
