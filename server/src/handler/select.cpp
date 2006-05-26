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
#include "storagebackend.h"
#include "storage/datastore.h"
#include "storage/entity.h"

#include "select.h"
#include "response.h"

using namespace Akonadi;

Select::Select(): Handler()
{
}


Select::~Select()
{
}


bool Select::handleLine(const QByteArray& line )
{
    // parse out the reference name and mailbox name
    int startOfCommand = line.indexOf( ' ' ) + 1;
    int startOfMailbox = line.indexOf( ' ', startOfCommand ) + 1;
    QByteArray mailbox = line.right( line.size() - startOfMailbox );
    qDebug() << "Select mailbox:" << mailbox << endl;

    Response response;
    response.setUntagged();

    Resource resource;
    DataStore *db = connection()->storageBackend();

    response.setSuccess();
    response.setTag( tag() );
    response.setString( "Select completed" );
    emit responseAvailable( response );
    deleteLater();
    return true;
}

