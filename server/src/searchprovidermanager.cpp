/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "emailsearchprovider.h"

#include "searchprovidermanager.h"

using namespace Akonadi;

SearchProviderManager* SearchProviderManager::mSelf = 0;

SearchProviderManager::SearchProviderManager()
{
}

SearchProviderManager::~SearchProviderManager()
{
}

SearchProviderManager* SearchProviderManager::self()
{
  if ( !mSelf )
    mSelf = new SearchProviderManager;

  return mSelf;
}

SearchProvider* SearchProviderManager::createSearchProviderForMimetype( const QByteArray &mimeType )
{
  SearchProvider *provider = 0;

  if ( mimeType == "message/rfc822" || mimeType == "message/news" ) {
    provider = new EmailSearchProvider;
  }

  return provider;
}
