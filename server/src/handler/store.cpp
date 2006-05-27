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

#include <QSqlQuery>
#include <QSqlError>
#include <QStringList>
#include <QUuid>
#include <QVariant> 

#include "akonadi.h"
#include "akonadiconnection.h"
#include "response.h"
#include "storequery.h"
#include "storage/datastore.h"

#include "store.h"

using namespace Akonadi;

Store::Store()
  : Handler()
{
}

Store::~Store()
{
}

bool Store::handleLine( const QByteArray& line )
{
  Response response;
  response.setUntagged();

  int start = line.indexOf( ' ' ) + 1; // skip tag

  StoreQuery storeQuery;

  if ( !storeQuery.parse( line.mid( start ) ) ) {
    response.setTag( tag() );
    response.setError();
    response.setString( "Syntax error" );

    emit responseAvailable( response );
    deleteLater();

    return true;
  }

  storeQuery.dump();
/*
  mDatabase = QSqlDatabase::addDatabase( "QSQLITE", QUuid::createUuid().toString() );
  mDatabase.setDatabaseName( "akonadi.db" );
  mDatabase.open();
  if ( !mDatabase.isOpen() ) {
    response.setTag( tag() );
    response.setError();
    response.setString( "Unable to open backend storage" );

    emit responseAvailable( response );
    deleteLater();

    return true;
  }
*/


  response.setTag( tag() );
  response.setSuccess();
  response.setString( "STORE completed" );

  emit responseAvailable( response );
  deleteLater();

  return true;
}


void Store::replaceFlags( const QString &uid, const QList<QByteArray> &flags )
{
  DataStore *store = connection()->storageBackend();

  PimItem item = store->pimItemById( uid.toInt() );
  if ( !item.isValid() ) {
    qDebug( "Store::replaceFlags: Invalid uid '%s' passed", qPrintable( uid ) );
    return;
  }

  QList<Flag> flagList;
  for ( int i = 0; i < flags.count(); ++i ) {
    Flag flag = store->flagByName( flags[ i ] );
    if ( !flag.isValid() ) {
      /**
       * If the flag does not exist we'll create it now.
       */
      if ( !store->appendFlag( flags[ i ] ) ) {
        qDebug( "Store::replaceFlags: Unable to add new flag '%s'", flags[ i ].data() );
        return;
      } else {
        flag = store->flagByName( flags[ i ] );
        if ( !flag.isValid() )
          return;
      }
    } else {
      flagList.append( flag );
    }
  }

  if ( !store->setItemFlags( item, flagList ) ) {
    qDebug( "Store::replaceFlags: Unable to set new item flags" );
    return;
  }
}

void Store::addFlags( const QString &uid, const QList<QByteArray> &flags )
{
  DataStore *store = connection()->storageBackend();

  PimItem item = store->pimItemById( uid.toInt() );
  if ( !item.isValid() ) {
    qDebug( "Store::addFlags: Invalid uid '%s' passed", qPrintable( uid ) );
    return;
  }

  QList<Flag> flagList;
  for ( int i = 0; i < flags.count(); ++i ) {
    Flag flag = store->flagByName( flags[ i ] );
    if ( !flag.isValid() ) {
      /**
       * If the flag does not exist we'll create it now.
       */
      if ( !store->appendFlag( flags[ i ] ) ) {
        qDebug( "Store::addFlags: Unable to add new flag '%s'", flags[ i ].data() );
        return;
      } else {
        flag = store->flagByName( flags[ i ] );
        if ( !flag.isValid() )
          return;
      }
    } else {
      flagList.append( flag );
    }
  }

  if ( !store->appendItemFlags( item, flagList ) ) {
    qDebug( "Store::addFlags: Unable to add new item flags" );
    return;
  }
}

void Store::deleteFlags( const QString &uid, const QList<QByteArray> &flags )
{
  DataStore *store = connection()->storageBackend();

  PimItem item = store->pimItemById( uid.toInt() );
  if ( !item.isValid() ) {
    qDebug( "Store::deleteFlags: Invalid uid '%s' passed", qPrintable( uid ) );
    return;
  }

  QList<Flag> flagList;
  for ( int i = 0; i < flags.count(); ++i ) {
    Flag flag = store->flagByName( flags[ i ] );
    if ( !flag.isValid() )
      continue;

    flagList.append( flag );
  }

  if ( !store->removeItemFlags( item, flagList ) ) {
    qDebug( "Store::deleteFlags: Unable to add new item flags" );
    return;
  }
}
