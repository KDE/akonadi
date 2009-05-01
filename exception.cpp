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

#include <KDebug>
#include <QString>

namespace Akonadi {

class Exception::Private
{
  public:
    QByteArray what;
    QByteArray assembledWhat;
};

Exception::Exception(const char* what) throw() :
  d( new Private )
{
  d->what = what;
}

Exception::Exception(const QByteArray& what) throw() :
  d( new Private )
{
  d->what = what;
}

Exception::Exception(const QString& what) throw() :
  d( new Private )
{
  d->what = what.toUtf8();
}

Exception::Exception(const Akonadi::Exception& other) throw() :
  std::exception( other ), d( new Private )
{
  d->what = other.d->what;
}

Exception::~Exception() throw()
{
  delete d;
}

QByteArray Exception::type() const throw()
{
  return "Akonadi::Exception";
}

const char* Exception::what() const throw()
{
  d->assembledWhat = QByteArray( type() + ": " + d->what );
  return d->assembledWhat.constData();
}

}
