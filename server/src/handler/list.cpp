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
#include <QDebug>

#include "akonadi.h"
#include "akonadiconnection.h"
#include "storage/datastore.h"
#include "storage/entity.h"

#include "list.h"
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
    int startOfCommand = line.indexOf( ' ' ) + 1;
    int startOfReference = line.indexOf( '"', startOfCommand ) + 1;
    int endOfReference = line.indexOf( '"', startOfReference );
    int startOfMailbox = line.indexOf( ' ', endOfReference ) + 1;
    QByteArray reference = line.mid( startOfReference, endOfReference - startOfReference );
    QByteArray mailbox = stripQuotes( line.right( line.size() - startOfMailbox ) );

    //qDebug() << "reference:" << reference << "mailbox:" << mailbox << "::" << endl;

    Response response;
    response.setUntagged();

    if ( mailbox.isEmpty() ) { // special case of asking for the delimiter
        response.setString( "LIST (\\Noselect) \"/\" \"\"" );
        emit responseAvailable( response );
    } else {
        DataStore *db = connection()->storageBackend();
        CollectionList collections = db->listCollections( reference, mailbox );
        CollectionListIterator it(collections);
        while ( it.hasNext() ) {
            Collection c = it.next();
            QString list( "LIST ");
            list += "(";
            bool first = true;
            if ( c.isNoSelect() ) {
                list += "\\Noselect";
                first = false;
            }
            if ( c.isNoInferiors() ) {
                if ( !first ) list += " ";
                list += "\\Noinferiors";
                first = false;
            }
            const QString supportedMimeTypes = c.getMimeTypes();
            if ( !supportedMimeTypes.isEmpty() ) { 
                if ( !first ) list += " ";
                list += "\\MimeTypes["+ c.getMimeTypes() +"]";
            }
            list += ") ";
            list += "\"/\" \""; // FIXME delimiter
            list += c.identifier();
            list += "\"";
            response.setString( list.toLatin1() );
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

