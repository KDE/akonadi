/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#include "list.h"

#include <QtCore/QDebug>

#include "akonadi.h"
#include "akonadiconnection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "handlerhelper.h"
#include "imapstreamparser.h"

#include "response.h"

using namespace Akonadi;

List::List(): Handler()
{
}


List::~List()
{
}


bool List::listCollections( const QString & prefix,
                                      const QString & mailboxPattern,
                                      QList<Collection> &result )
{
  bool rv = true;
  result.clear();

  if ( mailboxPattern.isEmpty() )
    return true;

  DataStore *db = connection()->storageBackend();
  const QString collectionDelimiter = db->collectionDelimiter();

  // normalize path and pattern
  QString sanitizedPattern( mailboxPattern );
  QString fullPrefix( prefix );
  const bool hasPercent = mailboxPattern.contains( QLatin1Char('%') );
  const bool hasStar = mailboxPattern.contains( QLatin1Char('*') );
  const int endOfPath = mailboxPattern.lastIndexOf( collectionDelimiter ) + 1;

  Resource resource;
  if ( fullPrefix.startsWith( QLatin1Char('#') ) ) {
    int endOfRes = fullPrefix.indexOf( collectionDelimiter );
    QString resourceName;
    if ( endOfRes < 0 ) {
      resourceName = fullPrefix.mid( 1 );
      fullPrefix = QString();
    } else {
      resourceName = fullPrefix.mid( 1, endOfRes - 1 );
      fullPrefix = fullPrefix.right( fullPrefix.length() - endOfRes );
    }

    qDebug() << "listCollections()" << resourceName;
    resource = Resource::retrieveByName( resourceName );
    qDebug() << "resource.isValid()" << resource.isValid();
    if ( !resource.isValid() ) {
      return false;
    }
  }

  if ( !mailboxPattern.startsWith( collectionDelimiter ) && fullPrefix != collectionDelimiter )
    fullPrefix += collectionDelimiter;
  fullPrefix += mailboxPattern.left( endOfPath );

  if ( hasPercent )
    sanitizedPattern = QLatin1String("%");
  else if ( hasStar )
    sanitizedPattern = QLatin1String("*");
  else
    sanitizedPattern = mailboxPattern.mid( endOfPath );

  qDebug() << "Resource: " << resource.name() << " fullPrefix: " << fullPrefix << " pattern: " << sanitizedPattern;

  if ( !fullPrefix.isEmpty() ) {
    rv = false;
  }

  QList<Collection> collections;
  if ( resource.isValid() )
    collections = Collection::retrieveFiltered( Collection::resourceIdColumn(), resource.id() );
  else
    collections = Collection::retrieveAll();

  foreach( const Collection &col, collections ) {
    const QString collection = collectionDelimiter + HandlerHelper::pathForCollection( col );
#if 0
    qDebug() << "Collection: " << collection << " col: " << col << " prefix: " << fullPrefix;
#endif
    const bool atFirstLevel =
      collection.lastIndexOf( collectionDelimiter ) == fullPrefix.lastIndexOf( collectionDelimiter );
    if ( collection.startsWith( fullPrefix ) ) {
      if ( hasStar || ( hasPercent && atFirstLevel ) ||
           collection == fullPrefix + sanitizedPattern ) {
        result.append( col );
      }
    }
    // Check, if requested folder has been found to distinguish between
    // non-existant folder and empty folder.
    if ( collection + collectionDelimiter == fullPrefix || fullPrefix == collectionDelimiter )
      rv = true;
  }

  return rv;
}

bool List::parseStream()
{
  QString reference = m_streamParser->readUtf8String();
  QString mailbox = m_streamParser->readUtf8String();

//     qDebug() << "reference:" << reference << "mailbox:" << mailbox << "::" << endl;

  Response response;
  response.setUntagged();

  if ( mailbox.isEmpty() ) { // special case of asking for the delimiter
    response.setString( "LIST (\\Noselect) \"/\" \"\"" );
    emit responseAvailable( response );
  } else {
    QList<Collection> collections;
    if ( !listCollections( reference, mailbox, collections ) ) {
      return failureResponse( "Unable to find collection" );
    }

    foreach ( const Collection &col, collections ) {
      QByteArray list( "LIST ");
      list += '(';
      bool first = true;
      QList<MimeType> supportedMimeTypes = col.mimeTypes();
      if ( supportedMimeTypes.isEmpty() ) {
        list += "\\Noinferiors";
        first = false;
      }
      bool canContainFolders = false;
      foreach ( const MimeType &mt, supportedMimeTypes ) {
        if ( mt.name() == QLatin1String("inode/directory") ) {
          canContainFolders = true;
          break;
        }
      }
      if ( canContainFolders ) {
        if ( !first ) list += ' ';
        list += "\\Noselect";
        first = false;
      }
      if ( !supportedMimeTypes.isEmpty() ) {
        if ( !first ) list += ' ';
        list += "\\MimeTypes[" + MimeType::joinByName( supportedMimeTypes, QLatin1String(",") ).toLatin1() + ']';
      }
      list += ") ";
      list += "\"/\" \""; // FIXME delimiter
      if ( col.isValid() )
        list += HandlerHelper::pathForCollection( col ).toUtf8();
      else
        list += col.name(); // search folder
      list += "\"";
      response.setString( list );
      emit responseAvailable( response );
    }
  }

  response.setSuccess();
  response.setTag( tag() );
  response.setString( "List completed" );
  emit responseAvailable( response );
  deleteLater();
  return true;
}

#include "list.moc"
