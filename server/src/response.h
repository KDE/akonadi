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
#ifndef AKONADIRESPONSE_H
#define AKONADIRESPONSE_H

#include <QtCore/QByteArray>
#include "akonadiprivate_export.h"

namespace Akonadi {

/**
    @brief A command result.
    Encapsulates the result of a command, including what to send
    back to the client.
 */
class AKONADIPRIVATE_EXPORT Response{
    enum ResultCode {
        OK = 0,
        NO = 1,
        BAD = 2,
        BYE = 3 // not strictly a result code, but easier to handle this way
    };
public:
    Response();

    ~Response();
    
    /** The response string to be sent to the client. */
    QByteArray asString() const;

    void setTag( const QByteArray& tag );
    void setUntagged();
    void setContinuation();

    void setString( const char* );
    void setString( const QByteArray& string );
    void setString( const QString& string );

    void setSuccess();
    void setFailure();
    void setError();
    void setBye();
private:
    QByteArray m_responseString;
    ResultCode m_resultCode;
    QByteArray m_tag;
};

}

#endif
