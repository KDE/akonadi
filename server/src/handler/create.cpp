/***************************************************************************
 *   Copyright (C) 2006 by Ingo Kloecker <kloecker@kde.org>                *
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
#include <QDebug>
#include <QStringList>

#include "akonadi.h"
#include "akonadiconnection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"
#include "handlerhelper.h"

#include "create.h"
#include "imapparser.h"
#include "response.h"

using namespace Akonadi;

Create::Create(): Handler()
{
}


Create::~Create()
{
}


bool Create::handleLine(const QByteArray& line )
{
    // parse out the reference name and mailbox name
    const int startOfCommand = line.indexOf( ' ' ) + 1;
    const int startOfMailbox = line.indexOf( ' ', startOfCommand ) + 1;
    QByteArray tmp;
    const int pos = PIM::ImapParser::parseQuotedString( line, tmp, startOfMailbox );
    QString mailbox = QString::fromUtf8( tmp );
    QList<QByteArray> mimeTypes;
    PIM::ImapParser::parseParenthesizedList( line, mimeTypes, pos );

    // strip off a trailing '/'
    mailbox = HandlerHelper::normalizeCollectionName( mailbox );

    // Responses:
    // OK - create completed
    // NO - create failure: can't create mailbox with that name
    // BAD - command unknown or arguments invalid
    Response response;
    response.setTag( tag() );

    if ( mailbox.isEmpty() || mailbox.contains( QLatin1String("//") ) ) {
        response.setError();
        response.setString( "Invalid argument" );
        emit responseAvailable( response );
        deleteLater();
        return true;
    }

    DataStore *db = connection()->storageBackend();
    Transaction transaction( db );

    const int startOfLocation = mailbox.indexOf( QLatin1Char('/') );
    const QString topLevelName = mailbox.left( startOfLocation );

    Location toplevel = db->locationByName( topLevelName );
    if ( !toplevel.isValid() )
        return failureResponse( "Cannot create folder " + mailbox.toUtf8() + " in  unknown toplevel folder " + topLevelName.toUtf8() );

    Resource resource = db->resourceById( toplevel.resourceId() );

    // first check whether location already exists
    if ( db->locationByName( mailbox ).isValid() )
        return failureResponse( "A folder with that name does already exist" );

    // we have to create all superior hierarchical folders, so look for the
    // starting point
    QStringList foldersToCreate;
    foldersToCreate.append( mailbox );
    for ( int endOfSupFolder = mailbox.lastIndexOf( QLatin1Char('/'), mailbox.size() - 1 );
          endOfSupFolder > 0;
          endOfSupFolder = mailbox.lastIndexOf( QLatin1Char('/'), endOfSupFolder - 2 ) ) {
        // check whether the superior hierarchical folder exists
        if ( ! db->locationByName( mailbox.left( endOfSupFolder ) ).isValid() ) {
            // the superior folder does not exist, so it has to be created
            foldersToCreate.prepend( mailbox.left( endOfSupFolder ) );
        }
        else {
            // the superior folder exists, so we can stop here
            break;
        }
    }

    // now we try to create all necessary folders
    // first check whether the existing superior folder can contain subfolders
    const int endOfSupFolder = foldersToCreate[0].lastIndexOf( QLatin1Char('/') );
    if ( endOfSupFolder > 0 ) {
        bool canContainSubfolders = false;
        const QList<MimeType> mimeTypes = db->mimeTypesForLocation( db->locationByName( mailbox.left( endOfSupFolder ) ).id() );
        foreach ( MimeType m, mimeTypes ) {
            if ( m.mimeType().toLower() == QLatin1String("inode/directory") ) {
                canContainSubfolders = true;
                break;
            }
        }
        if ( !canContainSubfolders )
            return failureResponse( "Superior folder cannot contain subfolders" );
    }
    // everything looks good, now we create the folders
    foreach ( QString folderName, foldersToCreate ) {
        int locationId = 0;
        if ( ! db->appendLocation( folderName, resource, &locationId ) )
            return failureResponse( "Adding " + folderName.toUtf8() + " to the database failed" );
        foreach ( QByteArray mimeType, mimeTypes ) {
            if ( !db->appendMimeTypeForLocation( locationId, QString::fromUtf8(mimeType) ) )
                return failureResponse( "Unable to append mimetype for collection." );
        }
    }

    if ( !transaction.commit() )
      return failureResponse( "Unable to commit transaction." );

    response.setSuccess();
    response.setString( "CREATE completed" );
    emit responseAvailable( response );

    deleteLater();
    return true;
}

