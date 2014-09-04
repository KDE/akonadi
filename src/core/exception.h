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

#include "akonadicore_export.h"

#include <QtCore/QByteArray>
#include <exception>

class QString;

namespace Akonadi {

/**
  Base class for exceptions used by the Akonadi library.
*/
class AKONADICORE_EXPORT Exception : public std::exception //krazy:exclude=dpointer
{
public:
    /**
      Creates a new exception with the error message @p what.
    */
    Exception(const char *what) throw();

    /**
      Creates a new exception with the error message @p what.
    */
    Exception(const QByteArray &what) throw();

    /**
      Creates a new exception with the error message @p what.
    */
    Exception(const QString &what) throw();

    /**
      Copy constructor.
    */
    Exception(const Exception &other) throw();

    /**
      Destructor.
    */
    virtual ~Exception() throw();

    /**
      Returns the error message associated with this exception.
    */
    const char *what() const throw();

    /**
      Returns the type of this exception.
    */
    virtual QByteArray type() const throw(); // ### Akonadi 2: return const char *

private:
    class Private;
    Private *d;
};

#define AKONADI_EXCEPTION_MAKE_TRIVIAL_INSTANCE( classname ) \
class AKONADICORE_EXPORT classname : public Akonadi::Exception \
{ \
  public: \
    classname ( const char *what ) throw() : Akonadi::Exception( what ) \
    { \
    } \
    classname ( const QByteArray &what ) throw() : Akonadi::Exception( what ) \
    { \
    } \
    classname ( const QString &what ) throw() : Akonadi::Exception( what ) \
    { \
    } \
    ~classname() throw(); \
    QByteArray type() const throw(); \
}

AKONADI_EXCEPTION_MAKE_TRIVIAL_INSTANCE(PayloadException);

#undef AKONADI_EXCEPTION_MAKE_TRIVIAL_INSTANCE

}

#endif
