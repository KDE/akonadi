/*******************************************************************************
**
** Filename   : mockobjects.h
** Created on : 29 May, 2006
** Copyright  : (c) 2006 Till Adam
** Email      : adam@kde.org
**
*******************************************************************************/

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
