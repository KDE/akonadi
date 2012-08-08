/*
  Copyright (C) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (C) 2010-2011 Sérgio Martins <iamsergio@gmail.com>

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

#include "utils_p.h"
#include <kpimutils/email.h>
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identity.h>
#include <kmime/kmime_header_parsing.h>

#include <KEMailSettings>

using namespace Akonadi::Calendar;

QString Akonadi::Calendar::fullName()
{
  KEMailSettings settings;
  QString tusername = settings.getSetting( KEMailSettings::RealName );

  // Quote the username as it might contain commas and other quotable chars.
  tusername = KPIMUtils::quoteNameIfNecessary( tusername );

  QString tname, temail;
  // ignore the return value from extractEmailAddressAndName() because
  // it will always be false since tusername does not contain "@domain".
  KPIMUtils::extractEmailAddressAndName( tusername, temail, tname );
  return tname;
}

QString Akonadi::Calendar::email()
{
  KEMailSettings emailSettings;
  return emailSettings.getSetting( KEMailSettings::EmailAddress );
}

bool Akonadi::Calendar::thatIsMe( const QString &_email )
{
  KPIMIdentities::IdentityManager identityManager( /*ro=*/ true );
  
  // NOTE: this method is called for every created agenda view item,
  // so we need to keep performance in mind

  /* identityManager()->thatIsMe() is quite expensive since it does parsing of
     _email in a way which is unnecessarily complex for what we can have here,
     so we do that ourselves. This makes sense since this

  if ( Akonadi::identityManager()->thatIsMe( _email ) ) {
    return true;
  }
  */

  // in case email contains a full name, strip it out.
  // the below is the simpler but slower version of the following code:
  // const QString email = KPIM::getEmailAddress( _email );
  const QByteArray tmp = _email.toUtf8();
  const char *cursor = tmp.constData();
  const char *end = tmp.data() + tmp.length();
  KMime::Types::Mailbox mbox;
  KMime::HeaderParsing::parseMailbox( cursor, end, mbox );
  const QString email = mbox.addrSpec().asString();

  KEMailSettings emailSettings;
  const QString myEmail = emailSettings.getSetting( KEMailSettings::EmailAddress );

  if ( myEmail == email ) {
    return true;
  }

  KPIMIdentities::IdentityManager::ConstIterator it;
  for ( it = identityManager.begin();
        it != identityManager.end(); ++it ) {
    if ( (*it).matchesEmailAddress( email ) ) {
      return true;
    }
  }

  // TODO: Remove the additional e-mails stuff from korganizer and test e-mail aliases
  return false;
}

QStringList Akonadi::Calendar::allEmails()
{
  KPIMIdentities::IdentityManager identityManager( /*ro=*/ true );
  // Grab emails from the email identities
  // Warning, this list could contain duplicates.
  return identityManager.allEmails();
}

