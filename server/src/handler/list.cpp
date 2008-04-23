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
#include "../../libs/imapparser_p.h"
#include "handlerhelper.h"

#include "response.h"

using namespace Akonadi;

List::List(): Handler()
{
}


List::~List()
{
}


bool List::handleLine(const QByteArray& line )
{
    // parse out the reference name and mailbox name
    int pos = line.indexOf( ' ' ) + 1; // skip tag
    pos = line.indexOf( ' ', pos ) + 1; // skip command
    QString reference;
    pos = ImapParser::parseString( line, reference, pos );
    QString mailbox;
    ImapParser::parseString( line, mailbox, pos );

//     qDebug() << "reference:" << reference << "mailbox:" << mailbox << "::" << endl;

    Response response;
    response.setUntagged();

    if ( mailbox.isEmpty() ) { // special case of asking for the delimiter
        response.setString( "LIST (\\Noselect) \"/\" \"\"" );
        emit responseAvailable( response );
    } else {
        QList<Location> collections;
        if ( !listCollections( reference, mailbox, collections ) ) {
          return failureResponse( "Unable to find collection" );
        }

        foreach ( Location loc, collections ) {
            QByteArray list( "LIST ");
            list += '(';
            bool first = true;
            QList<MimeType> supportedMimeTypes = loc.mimeTypes();
            if ( supportedMimeTypes.isEmpty() ) {
                list += "\\Noinferiors";
                first = false;
            }
            bool canContainFolders = false;
            foreach ( MimeType mt, supportedMimeTypes ) {
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
            if ( loc.isValid() )
              list += HandlerHelper::pathForCollection( loc ).toUtf8();
            else
              list += loc.name(); // search folder
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

bool List::listCollections( const QString & prefix,
                                      const QString & mailboxPattern,
                                      QList<Location> &result )
{
  bool rv = true;
  result.clear();

  if ( mailboxPattern.isEmpty() )
    return true;

  DataStore *db = connection()->storageBackend();
  const QString locationDelimiter = db->locationDelimiter();

  // normalize path and pattern
  QString sanitizedPattern( mailboxPattern );
  QString fullPrefix( prefix );
  const bool hasPercent = mailboxPattern.contains( QLatin1Char('%') );
  const bool hasStar = mailboxPattern.contains( QLatin1Char('*') );
  const int endOfPath = mailboxPattern.lastIndexOf( locationDelimiter ) + 1;

  Resource resource;
  if ( fullPrefix.startsWith( QLatin1Char('#') ) ) {
    int endOfRes = fullPrefix.indexOf( locationDelimiter );
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

  if ( !mailboxPattern.startsWith( locationDelimiter ) && fullPrefix != locationDelimiter )
    fullPrefix += locationDelimiter;
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

  QList<Location> locations;
  if ( resource.isValid() )
    locations = Location::retrieveFiltered( Location::resourceIdColumn(), resource.id() );
  else
    locations = Location::retrieveAll();

  foreach( Location l, locations ) {
    const QString location = locationDelimiter + HandlerHelper::pathForCollection( l );
#if 0
    qDebug() << "Location: " << location << " l: " << l << " prefix: " << fullPrefix;
#endif
    const bool atFirstLevel =
      location.lastIndexOf( locationDelimiter ) == fullPrefix.lastIndexOf( locationDelimiter );
    if ( location.startsWith( fullPrefix ) ) {
      if ( hasStar || ( hasPercent && atFirstLevel ) ||
           location == fullPrefix + sanitizedPattern ) {
        result.append( l );
      }
    }
    // Check, if requested folder has been found to distinguish between
    // non-existant folder and empty folder.
    if ( location + locationDelimiter == fullPrefix || fullPrefix == locationDelimiter )
      rv = true;
  }

  return rv;
}
