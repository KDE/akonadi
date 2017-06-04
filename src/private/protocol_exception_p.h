/*
    Copyright (c) 2015 Daniel Vrátil <dvratil@redhat.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

//krazy:excludeall=dpointer,inline

#ifndef AKONADI_PROTOCOLEXCEPTION_P_H
#define AKONADI_PROTOCOLEXCEPTION_P_H

#include "akonadiprivate_export.h"

#include <exception>

#include <QByteArray>

namespace Akonadi
{

class AKONADIPRIVATE_EXPORT ProtocolException : public std::exception
{
public:
    explicit ProtocolException(const char *what)
        : std::exception()
        , mWhat(what)
    {}

    const char *what() const throw() override {
        return mWhat.constData();
    }

private:
    QByteArray mWhat;
};
} // namespace Akonadi

#endif
