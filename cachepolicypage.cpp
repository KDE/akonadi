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

#include "cachepolicypage.h"

using namespace Akonadi;

CachePolicyPage::CachePolicyPage(QWidget * parent) :
    CollectionPropertiesPage( parent )
{
  setPageTitle( i18n( "&Cache" ) );
  ui.setupUi( this );
}

bool Akonadi::CachePolicyPage::canHandle(const Collection & collection) const
{
  return collection.type() != Collection::VirtualParent && collection.type() != Collection::Virtual;
}

void CachePolicyPage::load(const Collection & collection)
{
  CachePolicy policy = collection.cachePolicy();
  ui.inherit->setChecked( policy.inheritFromParent() );
  ui.checkInterval->setValue( policy.intervalCheckTime() );
  ui.localCacheTimeout->setValue( policy.cacheTimeout() );
  ui.syncOnDemand->setChecked( policy.syncOnDemand() );
  ui.localParts->setItems( policy.localParts() );
}

void CachePolicyPage::save(Collection & collection)
{
  CachePolicy policy = collection.cachePolicy();
  policy.setInheritFromParent( ui.inherit->isChecked() );
  policy.setIntervalCheckTime( ui.checkInterval->value() );
  policy.setCacheTimeout( ui.localCacheTimeout->value() );
  policy.enableSyncOnDemand( ui.syncOnDemand->isChecked() );
  policy.setLocalParts( ui.localParts->items() );
  collection.setCachePolicy( policy );
}
