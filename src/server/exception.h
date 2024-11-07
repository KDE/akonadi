/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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
    explicit Exception(const char *what) noexcept
        : mWhat(what)
    {
    }

    explicit Exception(const QByteArray &what) noexcept
        : mWhat(what)
    {
    }

    explicit Exception(const QString &what) noexcept
        : mWhat(what.toUtf8())
    {
    }

    Exception(const Exception &) = default;
    Exception &operator=(const Exception &) = default;

    ~Exception() override = default;

    const char *what() const noexcept override
    {
        return mWhat.constData();
    }

    virtual const char *type() const noexcept
    {
        return "General Exception";
    }

protected:
    QByteArray mWhat;
};

#define AKONADI_EXCEPTION_MAKE_INSTANCE(classname)                                                                                                             \
    class classname : public Akonadi::Server::Exception                                                                                                        \
    {                                                                                                                                                          \
    public:                                                                                                                                                    \
        classname(const char *what) noexcept                                                                                                                   \
            : Akonadi::Server::Exception(what)                                                                                                                 \
        {                                                                                                                                                      \
        }                                                                                                                                                      \
        classname(const QByteArray &what) noexcept                                                                                                             \
            : Akonadi::Server::Exception(what)                                                                                                                 \
        {                                                                                                                                                      \
        }                                                                                                                                                      \
        classname(const QString &what) noexcept                                                                                                                \
            : Akonadi::Server::Exception(what)                                                                                                                 \
        {                                                                                                                                                      \
        }                                                                                                                                                      \
        const char *type() const noexcept override                                                                                                             \
        {                                                                                                                                                      \
            return "" #classname;                                                                                                                              \
        }                                                                                                                                                      \
    }

} // namespace Server
} // namespace Akonadi
