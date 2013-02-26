/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 Bjoern Ricks <bjoern.ricks@intevation.de>

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

#include "qwincedialer.h"

#include <windows.h>
#include <Tapi.h>
#include <phone.h>

#include <klocalizedstring.h>

QWinCEDialer::QWinCEDialer(const QString &applicationName)
  : QDialer( applicationName )
{
}

QWinCEDialer::~QWinCEDialer()
{
}

bool QWinCEDialer::dialNumber(const QString &number)
{

  PHONEMAKECALLINFO phonecallinfo;
  WCHAR wnumber[TAPIMAXDESTADDRESSSIZE];
  long retval = -1;

  int numchars = number.toWCharArray( wnumber );
  wnumber[numchars] = '\0';

  memset( &phonecallinfo, 0, sizeof( phonecallinfo ) );
  phonecallinfo.cbSize = sizeof( phonecallinfo );
  phonecallinfo.dwFlags = PMCF_PROMPTBEFORECALLING;
  phonecallinfo.pszDestAddress = wnumber;
  phonecallinfo.pszAppName = NULL;
  phonecallinfo.pszCalledParty = NULL;
  phonecallinfo.pszComment = NULL;

  retval = PhoneMakeCall( &phonecallinfo );
  if ( retval != 0 ) {
    mErrorMessage = i18n( "Could not call phone number %1", number );
    return false;
  }
  return true;

}

bool QWinCEDialer::sendSms(const QString &number, const QString &text)
{
    //TODO: see http://msdn.microsoft.com/en-us/library/aa921968.aspx
    mErrorMessage = i18n( "Sending an SMS is currently not supported on WinCE" );
    return false;
}

