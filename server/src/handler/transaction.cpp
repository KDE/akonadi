/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "handler/transaction.h"
#include "storage/datastore.h"
#include "akonadiconnection.h"
#include "imapparser.h"
#include "response.h"

Akonadi::TransactionHandler::TransactionHandler()
{
}

Akonadi::TransactionHandler::~ TransactionHandler()
{
}

bool Akonadi::TransactionHandler::handleLine(const QByteArray & line)
{
  int pos = line.indexOf( ' ' ) + 1; // skip tag

  QByteArray command;
  pos = PIM::ImapParser::parseString( line, command, pos );

  DataStore *store = connection()->storageBackend();

  if ( command == "BEGIN" ) {
    if ( store->inTransaction() )
      return failureResponse( "There is already a transaction in progress." );
    if ( !store->beginTransaction() )
      return failureResponse( "Unable to begin transaction." );
  }

  if ( command == "ROLLBACK" ) {
    if ( !store->inTransaction() )
      return failureResponse( "There is no transaction in progress." );
    if ( !store->rollbackTransaction() )
      return failureResponse( "Unable to roll back transaction." );
  }

  if ( command == "COMMIT" ) {
    if ( !store->inTransaction() )
      return failureResponse( "There is not transaction in progress." );
    if ( !store->commitTransaction() )
      return failureResponse( "Unable to commit transaction." );
  }

  Response response;
  response.setTag( tag() );
  response.setSuccess();
  response.setString( command + " completed." );
  emit responseAvailable( response );

  return true;
}

