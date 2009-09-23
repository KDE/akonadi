/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "dialphonenumberaction.h"

#include "qskypedialer.h"

#include <kabc/phonenumber.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <krun.h>

using namespace Akonadi;

static QString strippedNumber( const QString &number )
{
  QString result;

  for ( int i = 0; i < number.length(); ++i ) {
    const QChar character = number.at( i );
    if ( character.isDigit() || (character == QLatin1Char( '+' ) && i == 0) )
      result += character;
  }

  return result;
}

void DialPhoneNumberAction::dialNumber( const KABC::PhoneNumber &number )
{
  KConfig config( QLatin1String( "akonadi_contactrc" ) );
  KConfigGroup group( &config, "Phone Dial Settings" );

  QString command;

  if ( number.type() & KABC::PhoneNumber::Fax )
    command = group.readEntry( "FaxCommandPattern", QString() );
  else
    command = group.readEntry( "PhoneCommandPattern", QString() );

  if ( command.isEmpty() ) {
    KMessageBox::sorry( 0, i18n( "There is no application set which could be executed. Please go to the settings dialog and configure one." ) );
    return;
  }

  // we handle skype separated
  if ( command == QLatin1String( "skype" ) ) {
    QSkypeDialer dialer( QLatin1String( "AkonadiContacts" ) );
    if ( !dialer.dialNumber( strippedNumber( number.number().trimmed() ) ) ) {
      KMessageBox::sorry( 0, dialer.errorMessage() );
    }
    return;
  }

  /*
   * %N the raw number
   * %n the number with all additional non-number characters removed
   */
  command = command.replace( QLatin1String( "%N" ), number.number() );
  command = command.replace( QLatin1String( "%n" ), strippedNumber( number.number().trimmed() ) );

  KRun::runCommand( command, 0 );
}
