/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#include "searchinstance.h"
#include "agentsearchinterface.h"
#include "akdbus.h"

using namespace Akonadi;

SearchInstance::SearchInstance( const QByteArray &id )
 : QObject()
 , mId( id )
 , mInterface( 0 )
{
}

SearchInstance::~SearchInstance()
{
}

bool SearchInstance::init()
{
  Q_ASSERT( !mInterface );

  mInterface = new OrgFreedesktopAkonadiAgentSearchInterface(
      AkDBus::agentServiceName( QString::fromLatin1( mId ), AkDBus::Preprocessor ),
      QLatin1String( "/" ),
      QDBusConnection::sessionBus(),
      this );

  if ( !mInterface || !mInterface->isValid() ) {
    delete mInterface;
    mInterface = 0;
    return false;
  }

  return true;
}

void SearchInstance::search( const QByteArray &searchId, const QString &query, qlonglong collectionId )
{
  mInterface->search( searchId, query, collectionId );
}

OrgFreedesktopAkonadiAgentSearchInterface* SearchInstance::interface() const
{
  return mInterface;
}
