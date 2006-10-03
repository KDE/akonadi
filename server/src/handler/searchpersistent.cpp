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
#include "akonadiconnection.h"
#include "searchpersistent.h"
#include "imapparser.h"
#include "response.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"

#include <QtCore/QStringList>

using namespace Akonadi;

SearchPersistent::SearchPersistent()
  : Handler()
{
}

SearchPersistent::~SearchPersistent()
{
}

bool SearchPersistent::handleLine( const QByteArray& line )
{
  int pos = line.indexOf( ' ' ) + 1; // skip tag
  QByteArray command;
  pos = ImapParser::parseString( line, command, pos );

  QString collectionName;
  pos = ImapParser::parseString( line, collectionName, pos );
  if ( collectionName.isEmpty() )
    return failureResponse( "No name specified" );

  DataStore *db = connection()->storageBackend();
  Transaction transaction( db );

  if ( command.toUpper() == "SEARCH_STORE" ) {

    PersistentSearch search = db->persistentSearch( collectionName );
    if ( search.isValid() )
      return failureResponse( "Persistent search does already exist" );

    QByteArray mimeType;
    pos = ImapParser::parseString( line, mimeType, pos );
    if ( mimeType.isEmpty() )
      return failureResponse( "No mimetype specified" );

    MimeType mt = db->mimeTypeByName( QString::fromUtf8(mimeType) );
    if ( !mt.isValid() )
      return failureResponse( "Invalid mimetype" );

    QByteArray queryString;
    ImapParser::parseString( line, queryString, pos );
    if ( queryString.isEmpty() )
      return failureResponse( "No query specified" );

    if ( !db->appendPersisntentSearch( collectionName, queryString ) )
      return failureResponse( "Unable to create persistent search" );

    // get the responsible search providers
    QStringList providers = providerForMimetype( mimeType );
    if ( providers.isEmpty() )
      return failureResponse( "No search providers found for this mimetype" );

    // call search providers
    qDebug() << "found search providers for mimetype:" << mimeType << providers;
    QList<QVariant> arguments;
    arguments << collectionName << QString::fromUtf8( queryString );
    DBusThread *dbusThread = static_cast<DBusThread*>( QThread::currentThread() );
    foreach ( QString provider, providers ) {
      QList<QVariant> result = dbusThread->callDBus( QLatin1String( "org.kde.Akonadi.SearchProvider." ) + provider,
                                     QLatin1String( "/" ),
                                     QLatin1String( "org.kde.Akonadi.SearchProvider" ),
                                     QLatin1String( "addSearch" ), arguments );
      qDebug() << "returned from search provider call: " << result;
    }

  } else if ( command.toUpper() == "SEARCH_DELETE" ) {

    PersistentSearch search = db->persistentSearch( collectionName );
    if ( !search.isValid() )
      return failureResponse( "No such persistent search" );

    // get the responsible search providers
    // TODO: fetch mimetype from the database
    QStringList providers = providerForMimetype( "message/rfc822" );
    if ( providers.isEmpty() )
      return failureResponse( "No search providers found for this mimetype" );

    // unregister at search providers
    QList<QVariant> arguments;
    arguments << collectionName;
    DBusThread *dbusThread = static_cast<DBusThread*>( QThread::currentThread() );
    foreach ( QString provider, providers ) {
      QList<QVariant> result = dbusThread->callDBus( QLatin1String( "org.kde.Akonadi.SearchProvider." ) + provider,
                                     QLatin1String( "/" ),
                                     QLatin1String( "org.kde.Akonadi.SearchProvider" ),
                                     QLatin1String( "removeSearch" ), arguments );
      qDebug() << "returned from search provider call: " << result;
    }

    if ( !db->removePersistentSearch( search ) )
      return failureResponse( "Unable to remove presistent search" );

  } else {
    return failureResponse( "Unknwon command" );
  }

  if ( !transaction.commit() )
    return failureResponse( "Unable to commit transaction" );

  Response response;
  response.setTag( tag() );
  response.setSuccess();
  response.setString( command + " completed" );
  emit responseAvailable( response );

  deleteLater();
  return true;
}
