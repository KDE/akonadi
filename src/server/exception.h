/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_EXCEPTION_H
#define AKONADI_EXCEPTION_H

#include <QByteArray>
#include <QString>
#include <exception>

namespace Akonadi
{
namespace Server
{

/**
  Base class for exception used internally by the Akonadi server.
*/
class Exception : public std::exception
{
public:
    explicit Exception(const char *what) throw()
        : mWhat(what)
    {
    }

    explicit Exception(const QByteArray &what) throw()
        : mWhat(what)
    {
    }

    explicit Exception(const QString &what) throw()
        : mWhat(what.toUtf8())
    {
    }

    Exception(const Exception &) = delete;
    Exception &operator=(const Exception &) = delete;

    virtual ~Exception() throw() = default;

    const char *what() const throw() override {
        return mWhat.constData();
    }

    virtual const char *type() const throw()
    {
        return "General Exception";
    }
protected:
    QByteArray mWhat;
};

#define AKONADI_EXCEPTION_MAKE_INSTANCE( classname ) \
    class classname : public Akonadi::Server::Exception \
    { \
    public: \
        classname ( const char *what ) throw() \
            : Akonadi::Server::Exception( what ) \
        { \
        } \
        classname ( const QByteArray &what ) throw() \
            : Akonadi::Server::Exception( what ) \
        { \
        } \
        classname ( const QString &what ) throw() \
            : Akonadi::Server::Exception( what ) \
        { \
        } \
        const char *type() const throw() override \
        { \
            return "" #classname; \
        } \
    }

} // namespace Server
} // namespace Akonadi

#endif
