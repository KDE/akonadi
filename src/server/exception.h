/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
