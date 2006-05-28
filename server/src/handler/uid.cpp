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

#include "akonadi.h"
#include "response.h"

#include "fetch.h"
#include "store.h"

#include "uid.h"

using namespace Akonadi;

Uid::Uid()
  : Handler()
{
}

Uid::~Uid()
{
}

bool Uid::handleLine( const QByteArray& line )
{
  int start = line.indexOf( ' ' ); // skip tag

  QByteArray subCommand;
  if ( !mSubHandler ) {
    start = line.indexOf( ' ', start + 1 );
    if ( start == -1 ) {
      Response response;
      response.setTag( tag() );
      response.setError();
      response.setString( "Syntax error" );

      emit responseAvailable( response );
      deleteLater();

      return true;
    }

    int end = line.indexOf( ' ', start + 1 );

    subCommand = line.mid( start + 1, end - start - 1 ).toUpper();

    mSubHandler = 0;
    if ( subCommand == "FETCH" )
      mSubHandler = new Fetch();
    else if ( subCommand == "STORE" )
      mSubHandler = new Store();

    if ( !mSubHandler ) {
      Response response;
      response.setTag( tag() );
      response.setError();
      response.setString( "Syntax error" );

      emit responseAvailable( response );
      deleteLater();

      return true;
    }

    mSubHandler->setTag( tag() );
    mSubHandler->setConnection( connection() );

    connect( mSubHandler, SIGNAL( responseAvailable( const Response & ) ),
             this, SIGNAL( responseAvailable( const Response & ) ) );
    connect( mSubHandler, SIGNAL( connectionStateChange( ConnectionState ) ),
             this, SIGNAL( connectionStateChange( ConnectionState ) ) );
  }

  if ( mSubHandler->handleLine( line ) )
    mSubHandler = 0;
  else
    return false;

  deleteLater();

  return true;
}
