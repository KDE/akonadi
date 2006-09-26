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

CollectionList MockBackend::listCollections( const QString& prefix,
                                             const QString& mailboxPattern ) const
{
    CollectionList list;
    //qDebug() << "Prefix: " << prefix << " pattern: " << mailboxPattern;
    if ( mailboxPattern == QLatin1String("%") ) {
        list << Collection( QLatin1String("INBOX") );
    } else if ( mailboxPattern == QLatin1String("*") ) {
        list << Collection( QLatin1String("INBOX") );
        list << Collection( QLatin1String("INBOX/foo") );
    } else if ( mailboxPattern.startsWith( QLatin1String("INBOX") ) ) {
        list << Collection( QLatin1String("foo") );
        list << Collection( QLatin1String("bar") );
    }
    return list;
}

