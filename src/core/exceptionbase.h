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

#ifndef AKONADI_EXCEPTIONBASE_H
#define AKONADI_EXCEPTIONBASE_H

#include "akonadicore_export.h"

#include <QByteArray>

#include <exception>
#include <memory>

class QString;

namespace Akonadi
{

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4275) // we are exporting a subclass of an unexported class, MSVC complains
#endif

/**
  Base class for exceptions used by the Akonadi library.
*/
class AKONADICORE_EXPORT Exception : public std::exception //krazy:exclude=dpointer
{
public:
    /**
      Creates a new exception with the error message @p what.
    */
    explicit Exception(const char *what);

    /**
      Creates a new exception with the error message @p what.
    */
    explicit Exception(const QByteArray &what);

    /**
      Creates a new exception with the error message @p what.
    */
    explicit Exception(const QString &what);

    /**
      Copy constructor.
    */
    Exception(const Exception &other);

    Exception(Exception &&other) = default;

    /**
      Destructor.
    */
    ~Exception()  override;

    /**
      Returns the error message associated with this exception.
    */
    const char *what() const noexcept override;

    /**
      Returns the type of this exception.
    */
    virtual QByteArray type() const; // ### Akonadi 2: return const char *

private:
    class Private;
    std::unique_ptr<Private> d;
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#define AKONADI_EXCEPTION_MAKE_TRIVIAL_INSTANCE(classname) \
    class AKONADICORE_EXPORT classname : public Akonadi::Exception \
    { \
    public: \
        explicit classname(const char *what): Akonadi::Exception(what) {} \
        explicit classname(const QByteArray &what): Akonadi::Exception(what) {} \
        explicit classname(const QString &what): Akonadi::Exception(what) {} \
        ~classname() override; \
        QByteArray type() const override; \
    }

AKONADI_EXCEPTION_MAKE_TRIVIAL_INSTANCE(PayloadException);

#undef AKONADI_EXCEPTION_MAKE_TRIVIAL_INSTANCE

}

#endif
