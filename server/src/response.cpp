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
#include <QtCore/QTextStream>

#include "response.h"

using namespace Akonadi;

static const char* s_resultCodeStrings[] = {
    "OK", "NO", "BAD", "BYE", ""
};

Response::Response()
    : m_resultCode( OK )
    , m_tag( "*" )
{
}


Response::~Response()
{
}

QByteArray Response::asString() const
{
    QByteArray b = m_tag;
    if ( m_tag != "*" && m_tag != "+" && m_resultCode != USER ) {
        b += " ";
        b += s_resultCodeStrings[m_resultCode];
    }
    b += " ";
    b += m_responseString;
    return b;
}

void Response::setSuccess( )
{
    m_resultCode = Response::OK;
}

void Response::setFailure( )
{
    m_resultCode = Response::NO;
}

void Response::setError( )
{
    m_resultCode = Response::BAD;
}

void Response::setTag( const QByteArray& tag )
{
    m_tag = tag;
}

void Response::setUntagged( )
{
    m_tag = "*";
}

void Response::setContinuation( )
{
    m_tag = "+";
}

void Response::setString( const QByteArray & string )
{
    m_responseString = string;
}

void Akonadi::Response::setString( const QString & string )
{
    m_responseString = string.toLatin1();
}


void Akonadi::Response::setString( const char *string )
{
    m_responseString = QByteArray( string );
}

void Response::setBye( )
{
    m_resultCode = Response::BYE;
}

void Response::setUserDefined()
{
  m_resultCode = Response::USER;
}
