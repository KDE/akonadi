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

#include "cachepolicy.h"
#include "collection.h"
#include "collectionutils_p.h"

//@cond PRIVATE

using namespace Akonadi;

CachePolicyPage::CachePolicyPage(QWidget * parent) :
    CollectionPropertiesPage( parent )
{
  setPageTitle( i18n( "Cache" ) );
  ui.setupUi( this );
  connect( ui.checkInterval, SIGNAL( valueChanged( int ) ),
           SLOT( slotIntervalValueChanged( int ) ) );
  connect( ui.localCacheTimeout, SIGNAL( valueChanged( int ) ),
           SLOT( slotCacheValueChanged( int ) ) );
}

bool Akonadi::CachePolicyPage::canHandle(const Collection & collection) const
{
  return !CollectionUtils::isVirtual( collection );
}

void CachePolicyPage::load(const Collection & collection)
{
  CachePolicy policy = collection.cachePolicy();

  int interval = policy.intervalCheckTime();
  if (interval == -1)
    interval = 0;

  int cache = policy.cacheTimeout();
  if (cache == -1)
    cache = 0;

  ui.inherit->setChecked( policy.inheritFromParent() );
  ui.checkInterval->setValue( interval );
  ui.localCacheTimeout->setValue( cache );
  ui.syncOnDemand->setChecked( policy.syncOnDemand() );
  ui.localParts->setItems( policy.localParts() );
}

void CachePolicyPage::save(Collection & collection)
{
  int interval = ui.checkInterval->value();
  if (interval == 0)
    interval = -1;

  int cache = ui.localCacheTimeout->value();
  if (cache == 0)
    cache = -1;

  CachePolicy policy = collection.cachePolicy();
  policy.setInheritFromParent( ui.inherit->isChecked() );
  policy.setIntervalCheckTime( interval );
  policy.setCacheTimeout( cache );
  policy.setSyncOnDemand( ui.syncOnDemand->isChecked() );
  policy.setLocalParts( ui.localParts->items() );
  collection.setCachePolicy( policy );
}

void CachePolicyPage::slotIntervalValueChanged( int i )
{
    ui.checkInterval->setSuffix( QLatin1Char(' ') + i18np( "minute", "minutes", i ) );
}

void CachePolicyPage::slotCacheValueChanged( int i )
{
    ui.localCacheTimeout->setSuffix( QLatin1Char(' ') + i18np( "minute", "minutes", i ) );
}

//@endcond

#include "cachepolicypage.moc"
