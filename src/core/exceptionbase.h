/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QByteArray>

#include <exception>
#include <memory>

class QString;

namespace Akonadi
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4275) // we are exporting a subclass of an unexported class, MSVC complains
#endif

class ExceptionPrivate;
/*!
 * \class Akonadi::Exception
 * \inheaders Akonadi/ExceptionBase
 * \inmodule AkonadiCore
 *
 * Base class for exceptions used by the Akonadi library.
 */
class AKONADICORE_EXPORT Exception : public std::exception
{
public:
    /*!
      Creates a new exception with the error message \a what.
    */
    explicit Exception(const char *what);

    /*!
      Creates a new exception with the error message \a what.
    */
    explicit Exception(const QByteArray &what);

    /*!
      Creates a new exception with the error message \a what.
    */
    explicit Exception(const QString &what);

    Exception(Exception &&) noexcept;

    /*!
      Destructor.
    */
    ~Exception() override;

    /*!
      Returns the error message associated with this exception.
    */
    const char *what() const noexcept override;

    /*!
      Returns the type of this exception.
    */
    virtual QByteArray type() const; // ### Akonadi 2: return const char *

private:
    std::unique_ptr<ExceptionPrivate> d;
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#define AKONADI_EXCEPTION_MAKE_TRIVIAL_INSTANCE(classname)                                                                                                     \
    class AKONADICORE_EXPORT classname : public Akonadi::Exception                                                                                             \
    {                                                                                                                                                          \
    public:                                                                                                                                                    \
        explicit classname(const char *what)                                                                                                                   \
            : Akonadi::Exception(what)                                                                                                                         \
        {                                                                                                                                                      \
        }                                                                                                                                                      \
        explicit classname(const QByteArray &what)                                                                                                             \
            : Akonadi::Exception(what)                                                                                                                         \
        {                                                                                                                                                      \
        }                                                                                                                                                      \
        explicit classname(const QString &what)                                                                                                                \
            : Akonadi::Exception(what)                                                                                                                         \
        {                                                                                                                                                      \
        }                                                                                                                                                      \
        classname(classname &&) = default;                                                                                                                     \
        ~classname() override;                                                                                                                                 \
        QByteArray type() const override;                                                                                                                      \
    }

AKONADI_EXCEPTION_MAKE_TRIVIAL_INSTANCE(PayloadException);

#undef AKONADI_EXCEPTION_MAKE_TRIVIAL_INSTANCE

}
