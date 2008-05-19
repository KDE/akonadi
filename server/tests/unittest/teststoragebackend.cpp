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

#include "teststoragebackend.h"

using namespace Akonadi;

bool MockBackend::listCollections( const QString& prefix,
                                   const QString& mailboxPattern,
                                   QList<Location> &result ) const
{
    result.clear();
    //qDebug() << "Prefix: " << prefix << " pattern: " << mailboxPattern;
    if ( mailboxPattern == QLatin1String("%") ) {
        Location l;
        l.setName( "INBOX" );
        result << l;
    } else if ( mailboxPattern == QLatin1String("*") ) {
        Location l1; l1.setName( "INBOX" );
        Location l2; l2.setName( "INBOX/foo" );
        result << l1 << l2;
    } else if ( mailboxPattern.startsWith( "INBOX" ) ) {
        Location l1; l1.setName( "foo" );
        Location l2; l2.setName( "bar" );
        result << l1 << l2;
    }
    return true;
}

