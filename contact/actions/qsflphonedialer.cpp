/*
  Copyright (c) 2013 Montel Laurent <montel.org>

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

#include "qsflphonedialer.h"

#include <KLocale>

QSflPhoneDialer::QSflPhoneDialer( const QString &applicationName )
  : QDialer( applicationName )
{
}

QSflPhoneDialer::~QSflPhoneDialer()
{
}

bool QSflPhoneDialer::dialNumber(const QString &number)
{
  return true;
}

bool QSflPhoneDialer::sendSms(const QString &, const QString &)
{
    mErrorMessage = i18n( "Sending an SMS is currently not supported on SflPhone" );
    return false;
}
