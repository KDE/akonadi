/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "message.h"

#include <kmime_message.h>

using namespace PIM;

class Message::Private
{
  public:
    DataReference ref;
    KMime::Message *mime;
};


PIM::Message::Message( const DataReference & ref ) :
    d( new Private() )
{
  d->ref = ref;
}

PIM::Message::~Message( )
{
  delete d->mime;
  delete d;
}

DataReference PIM::Message::reference( ) const
{
  return d->ref;
}

KMime::Message * PIM::Message::mime( ) const
{
  return d->mime;
}

void PIM::Message::setMime( KMime::Message * mime )
{
  d->mime = mime;
}
