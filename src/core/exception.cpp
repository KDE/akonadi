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

#include "exception.h"

#include <QString>

#include <memory>

using namespace Akonadi;

class Exception::Private
{
public:
    QByteArray what;
    QByteArray assembledWhat;
};

Exception::Exception(const char *what) throw()
    : d(0)
{
    try {
        std::unique_ptr<Private> nd(new Private);
        nd->what = what;
        d = nd.release();
    } catch (...) {
    }
}

Exception::Exception(const QByteArray &what) throw()
    : d(0)
{
    try {
        std::unique_ptr<Private> nd(new Private);
        nd->what = what;
        d = nd.release();
    } catch (...) {
    }
}

Exception::Exception(const QString &what) throw()
    : d(0)
{
    try {
        std::unique_ptr<Private> nd(new Private);
        nd->what = what.toUtf8();
        d = nd.release();
    } catch (...) {
    }
}

Exception::Exception(const Akonadi::Exception &other) throw()
    : std::exception(other)
    , d(0)
{
    if (!other.d) {
        return;
    }
    try {
        std::unique_ptr<Private> nd(new Private(*other.d));
        d = nd.release();
    } catch (...) {
    }
}

Exception::~Exception() throw()
{
    delete d;
}

QByteArray Exception::type() const throw()
{
    static const char mytype[] = "Akonadi::Exception";
    try {
        return QByteArray::fromRawData("Akonadi::Exception", sizeof (mytype) - 1);
    } catch (...) {
        return QByteArray();
    }
}

const char *Exception::what() const throw()
{
    static const char fallback[] = "<some exception was thrown during construction: message lost>";
    if (!d) {
        return fallback;
    }
    if (d->assembledWhat.isEmpty()) {
        try {
            d->assembledWhat = QByteArray(type() + ": " + d->what);
        } catch (...) {
            return "caught some exception while assembling Akonadi::Exception::what() return value";
        }
    }
    return d->assembledWhat.constData();
}

#define AKONADI_EXCEPTION_IMPLEMENT_TRIVIAL_INSTANCE( classname )       \
    Akonadi::classname::~classname() throw() {}                         \
    QByteArray Akonadi::classname::type() const throw() {               \
        static const char mytype[] = "Akonadi::" #classname ;           \
        try {                                                           \
            return QByteArray::fromRawData( mytype, sizeof (mytype)-1 ); \
        } catch ( ... ) {                                               \
            return QByteArray();                                        \
        }                                                               \
    }

AKONADI_EXCEPTION_IMPLEMENT_TRIVIAL_INSTANCE(PayloadException)

#undef AKONADI_EXCEPTION_IMPLEMENT_TRIVIAL_INSTANCE
