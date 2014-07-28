/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef TESTATTRIBUTE_H
#define TESTATTRIBUTE_H

#include "attribute.h"

#include <QByteArray>

/* Attribute used for testing by various unit tests. */
class TestAttribute : public Akonadi::Attribute
{
  public:
    TestAttribute(){};
    QByteArray type() const { return "EXTRA"; }
    QByteArray serialized() const { return data; }
    void deserialize( const QByteArray &ba ) { data = ba; }
    TestAttribute* clone() const { TestAttribute* a = new TestAttribute; a->data = data; return a; }
    QByteArray data;
};

#endif
