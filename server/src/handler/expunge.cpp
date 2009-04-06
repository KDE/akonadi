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

#include "expunge.h"

#include "akonadi.h"
#include "akonadiconnection.h"
#include "response.h"
#include "storage/datastore.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"
#include "imapstreamparser.h"

using namespace Akonadi;

Expunge::Expunge()
  : Handler()
{
}

Expunge::~Expunge()
{
}

bool Expunge::parseStream()
{
  Response response;

  DataStore *store = connection()->storageBackend();
  Transaction transaction( store );

  Flag flag = Flag::retrieveByName( QLatin1String("\\Deleted") );
  if ( !flag.isValid() ) {
    response.setError();
    response.setString( "\\Deleted flag unknown" );

    emit responseAvailable( response );
    deleteLater();

    return true;
  }

  SelectQueryBuilder<PimItem> qb;
  qb.addTable( PimItemFlagRelation::tableName() );
  qb.addColumnCondition( PimItem::idFullColumnName(), Query::Equals, PimItemFlagRelation::leftFullColumnName() );
  qb.addValueCondition( PimItemFlagRelation::rightFullColumnName(), Query::Equals, flag.id() );

  if ( qb.exec() ) {
    const QList<PimItem> items = qb.result();
    foreach ( const PimItem &item, items ) {
      if ( store->cleanupPimItem( item ) ) {
        response.setUntagged();
        // IMAP protocol violation: should actually be the sequence number
        response.setString( QByteArray::number( item.id() ) + " EXPUNGE" );

        emit responseAvailable( response );
      } else {
        response.setTag( tag() );
        response.setError();
        response.setString( "internal error" );

        emit responseAvailable( response );
        deleteLater();
        return true;
      }
    }
  } else {
    throw HandlerException( "Unable to execute query." );
  }

  if ( !transaction.commit() )
    return failureResponse( "Unable to commit transaction." );

  response.setTag( tag() );
  response.setSuccess();
  response.setString( "EXPUNGE completed" );

  emit responseAvailable( response );
  deleteLater();

  return true;
}

#include "expunge.moc"
