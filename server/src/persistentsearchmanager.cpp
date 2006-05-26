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

#include <QStringList>

#include "persistentsearchmanager.h"

using namespace Akonadi;

PersistentSearchManager* PersistentSearchManager::mSelf = 0;

PersistentSearchManager::PersistentSearchManager()
{
}

PersistentSearchManager::~PersistentSearchManager()
{
}

PersistentSearchManager* PersistentSearchManager::self()
{
  if ( !mSelf )
    mSelf = new PersistentSearchManager;

  return mSelf;
}

void PersistentSearchManager::addPersistentSearch( const QString &identifier, PersistentSearch *search )
{
  mListMutex.lock();
  if ( mList.values().contains( search ) ) {
    qWarning( "PersistentSearchManager: tried to add the same persistent search twice!" );
    mListMutex.unlock();
    return;
  }

  if ( mList.contains( identifier ) ) {
    qWarning( "PersistentSearchManager: tried to add the same identifier twice!" );
    mListMutex.unlock();
    return;
  }

  mList.insert( identifier, search );

  mListMutex.unlock();
}

void PersistentSearchManager::removePersistentSearch( const QString &identifier )
{
  mListMutex.lock();

  if ( !identifier.contains( identifier ) ) {
    mListMutex.unlock();
    return;
  }

  delete mList.value( identifier );
  mList.remove( identifier );

  mListMutex.unlock();
}

QList<QByteArray> PersistentSearchManager::uids( const QString &identifier ) const
{
  QList<QByteArray> uids;

  mListMutex.lock();
  if ( mList.contains( identifier ) ) {
    PersistentSearch *search = mList.value( identifier );
    uids = search->uids();
  }
  mListMutex.unlock();

  return uids;
}

QList<QByteArray> PersistentSearchManager::objects( const QString &identifier ) const
{
  QList<QByteArray> objects;

  mListMutex.lock();
  if ( mList.contains( identifier ) ) {
    PersistentSearch *search = mList.value( identifier );
    objects = search->objects();
  }
  mListMutex.unlock();

  return objects;
}

CollectionList PersistentSearchManager::collections() const
{
  CollectionList collections;

  QStringList identifiers;
  mListMutex.lock();
  identifiers = mList.keys();
  mListMutex.unlock();

  for ( int i = 0; i < identifiers.count(); ++i )
    collections.append( Collection( identifiers[ i ] ) );

  return collections;
}
