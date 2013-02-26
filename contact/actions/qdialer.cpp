/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>
                  2010 Bjoern Ricks  <bjoern.ricks@intevation.de>

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

#include "qdialer.h"

#include <klocalizedstring.h>

QDialer::QDialer( const QString &applicationName)
  : mApplicationName( applicationName )
{
}

QDialer::~QDialer()
{
}

bool QDialer::dialNumber( const QString &number )
{
  mErrorMessage = i18n( "Dialing a number is not supported" );

  return false;
}

bool QDialer::sendSms( const QString &number, const QString &text )
{
  mErrorMessage = i18n( "Sending an SMS is not supported" );

  return false;
}

QString QDialer::errorMessage() const
{
  return mErrorMessage;
}
