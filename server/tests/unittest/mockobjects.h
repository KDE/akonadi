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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef MOCKOBJECTS_H
#define MOCKOBJECTS_H

#include "akonadiconnection.h"
#include "teststoragebackend.h"

using namespace Akonadi;

static AkonadiConnection * s_connection = 0;
static DataStore * s_backend = 0;

class MockConnection : public AkonadiConnection
{
public:
    MockConnection()
    {
    }
    DataStore* storageBackend()
    {
        if ( !s_backend )
            s_backend = new MockBackend();
        return s_backend; 
    }
};

class MockObjects
{
public:
	MockObjects();
	~MockObjects();

        static AkonadiConnection * mockConnection()
        {
           if ( !s_connection )
               s_connection = new MockConnection();
           return s_connection; 
        }
}; // End of class MockObjects


#endif // MOCKOBJECTS_H
