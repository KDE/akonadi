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

#include "persistentsearch.h"

using namespace Akonadi;

PersistentSearch::PersistentSearch( const QList<QByteArray> &query, SearchProvider *provider )
  : mQuery( query ), mProvider( provider )
{
}

PersistentSearch::~PersistentSearch()
{
  delete mProvider;
  mProvider = 0;
}

QList<QByteArray> PersistentSearch::uids( const DataStore *store ) const
{
  return mProvider->queryForUids( mQuery, store );
}

QList<QByteArray> PersistentSearch::objects( const DataStore *store ) const
{
  return mProvider->queryForObjects( mQuery, store );
}

QList<QByteArray> PersistentSearch::searchCriteria() const
{
  return mQuery;
}
