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

#include "uid.h"

#include "akonadi.h"
#include "response.h"

#include "aklist.h"
#include "fetch.h"
#include "select.h"
#include "store.h"

#include "imapstreamparser.h"

#include <QDebug>

using namespace Akonadi;

Uid::Uid( Scope::SelectionScope scope ) :
  Handler(),
  mScope( scope )
{
}

Uid::~Uid()
{
}


bool Uid::parseStream()
{
  if ( !mSubHandler ) {
    if ( !m_streamParser->hasString() )
      throw HandlerException( "Syntax error" );

    const QByteArray subCommand = m_streamParser->readString().toUpper();

    mSubHandler = 0;
    if ( subCommand == "FETCH" )
      mSubHandler = new Fetch( mScope );
    else if ( subCommand == "STORE" )
      mSubHandler = new Store( true );
    else if ( subCommand == "SELECT" )
      mSubHandler = new Select( mScope );
    else if ( subCommand == "X-AKLIST" )
      mSubHandler = new AkList( mScope, false );
    else if ( subCommand == "X-AKLSUB" )
      mSubHandler = new AkList( mScope, true );

    if ( !mSubHandler )
      throw HandlerException( "Unknown UID/RID subcommand" );

    mSubHandler->setStreamParser( m_streamParser );
    mSubHandler->setTag( tag() );
    mSubHandler->setConnection( connection() );

    connect( mSubHandler, SIGNAL( responseAvailable( const Response & ) ),
             this, SIGNAL( responseAvailable( const Response & ) ) );
    connect( mSubHandler, SIGNAL( connectionStateChange( ConnectionState ) ),
             this, SIGNAL( connectionStateChange( ConnectionState ) ) );
  }

  bool result = true;
  try {
    result = mSubHandler->parseStream();
  }
  catch ( const Akonadi::HandlerException &e ) {
    mSubHandler->failureResponse( e.what() );
    mSubHandler->deleteLater();
    result = false;
  }
  mSubHandler = 0;
  deleteLater();

  return result;
}

#include "uid.moc"
