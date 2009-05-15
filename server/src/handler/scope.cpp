/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "scope.h"
#include "imapstreamparser.h"
#include "handler.h"

#include <cassert>

using namespace Akonadi;

Scope::Scope( SelectionScope scope ) :
  mScope( scope )
{
}

void Scope::parseScope( ImapStreamParser* parser )
{
  if ( mScope == None || mScope == Uid ) {
    mUidSet = parser->readSequenceSet();
    if ( mUidSet.isEmpty() )
      throw HandlerException( "Empty uid set specified" );
  } else if ( mScope == Rid ) {
    if ( parser->hasList() ) {
      parser->beginList();
      while ( !parser->atListEnd() )
        mRidSet << parser->readUtf8String();
    } else {
      mRidSet << parser->readUtf8String();
    }
    if ( mRidSet.isEmpty() )
      throw HandlerException( "Empty remote identifier set specified" );
  } else {
    throw HandlerException( "WTF?!?" );
  }
}

Scope::SelectionScope Scope::scope() const
{
  return mScope;
}

void Scope::setScope( SelectionScope scope )
{
  mScope = scope;
}

ImapSet Scope::uidSet() const
{
  return mUidSet;
}

void Scope::setUidSet(const ImapSet& set)
{
  mUidSet = set;
}

QStringList Scope::ridSet() const
{
  return mRidSet;
}
