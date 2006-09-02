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

#include "akonadi.h"
#include "akonadiconnection.h"
#include "storage/datastore.h"
#include "storage/entity.h"

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
    QByteArray mailbox;
    const int pos = PIM::ImapParser::parseQuotedString( line, mailbox, startOfMailbox );
    QList<QByteArray> mimeTypes;
    PIM::ImapParser::parseParenthesizedList( line, mimeTypes, pos );
    qDebug() << "mailbox: " << mailbox << " mimeTypes: " << mimeTypes << " pos: " << pos;

    // strip off a trailing '/'
    if ( !mailbox.isEmpty() && mailbox[0] == '/' )
      mailbox = mailbox.right( mailbox.size() - 1 );

    // Responses:
    // OK - create completed
    // NO - create failure: can't create mailbox with that name
    // BAD - command unknown or arguments invalid
    Response response;
    response.setTag( tag() );

    if ( mailbox.isEmpty() || mailbox.contains( "//" ) ) {
        response.setError();
        response.setString( "Invalid argument" );
        emit responseAvailable( response );
        deleteLater();
        return true;
    }

    DataStore *db = connection()->storageBackend();
    const int startOfLocation = mailbox.indexOf( '/' );
    const QByteArray topLevelName = mailbox.left( startOfLocation );

    Location toplevel = db->locationByName( topLevelName );
    if ( !toplevel.isValid() )
        return failureResponse( "Cannot create folder " + mailbox + " in  unknown toplevel folder " + topLevelName );

    Resource resource = db->resourceById( toplevel.resourceId() );

    // first check whether location already exists
    if ( db->locationByName( mailbox ).isValid() )
        return failureResponse( "A folder with that name does already exist" );

    // we have to create all superior hierarchical folders, so look for the
    // starting point
    QList<QByteArray> foldersToCreate;
    foldersToCreate.append( mailbox );
    for ( int endOfSupFolder = mailbox.lastIndexOf( '/', mailbox.size() - 1 );
          endOfSupFolder > 0;
          endOfSupFolder = mailbox.lastIndexOf( '/', endOfSupFolder - 2 ) ) {
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
    const int endOfSupFolder = foldersToCreate[0].lastIndexOf( '/' );
    if ( endOfSupFolder > 0 ) {
        bool canContainSubfolders = false;
        const QList<MimeType> mimeTypes = db->mimeTypesForLocation( db->locationByName( mailbox.left( endOfSupFolder ) ).id() );
        foreach ( MimeType m, mimeTypes ) {
            if ( m.mimeType().toLower() == "inode/directory" ) {
                canContainSubfolders = true;
                break;
            }
        }
        if ( !canContainSubfolders )
            return failureResponse( "Superior folder cannot contain subfolders" );
    }
    // everything looks good, now we create the folders
    foreach ( QByteArray folderName, foldersToCreate ) {
        int locationId = 0;
        if ( ! db->appendLocation( folderName, resource, &locationId ) )
            return failureResponse( "Adding " + folderName + " to the database failed" );
        // FIXME what to do if appending the mime type fails?
        foreach ( QByteArray mimeType, mimeTypes )
          db->appendMimeTypeForLocation( locationId, QString::fromUtf8(mimeType) );
    }

    response.setSuccess();
    response.setString( "CREATE completed" );
    emit responseAvailable( response );

    deleteLater();
    return true;
}

