/*
  Copyright (c) 1998 Barry D Benowitz <b.benowitz@telesciences.com>
  Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2009 Allen Winter <winter@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/
#ifndef AKONADI_MAILCLIENT_P_H
#define AKONADI_MAILCLIENT_P_H

#include <kcalcore/incidencebase.h>

#include <QObject>

#ifdef MAILCLIENTTEST_UNITTEST
#include <kmime/kmime_message.h>
  struct UnitTestResult {
    QString from;
    QStringList to;
    QStringList cc;
    QStringList bcc;
    int transportId;
    KMime::Message::Ptr message;
  };
#endif

namespace KPIMIdentities {
  class Identity;
}

class KJob;

namespace Akonadi {

class MailClient : public QObject
{
  Q_OBJECT
  public:

    enum Result {
      ResultSuccess,
      ResultNoAttendees,
      ResultReallyNoAttendees,
      ResultErrorCreatingTransport,
      ResultErrorFetchingTransport,
      ResultQueueJobError
    };

    explicit MailClient( QObject *parent = 0 );
    ~MailClient();

    void mailAttendees( const KCalCore::IncidenceBase::Ptr &,
                        const KPIMIdentities::Identity &identity,
                        bool bccMe, const QString &attachment=QString(),
                        const QString &mailTransport = QString() );

    void mailOrganizer( const KCalCore::IncidenceBase::Ptr &,
                        const KPIMIdentities::Identity &identity,
                        const QString &from, bool bccMe,
                        const QString &attachment=QString(),
                        const QString &sub=QString(),
                        const QString &mailTransport = QString() );

    void mailTo( const KCalCore::IncidenceBase::Ptr &, const KPIMIdentities::Identity &identity,
                 const QString &from, bool bccMe, const QString &recipients,
                 const QString &attachment=QString(), const QString &mailTransport = QString() );

    /**
      Sends mail with specified from, to and subject field and body as text.
      If bcc is set, send a blind carbon copy to the sender

      @param identity is the Identity of the sender
      @param from is the address of the sender of the message
      @param to a list of addresses to receive the message
      @param cc a list of addresses to receive message carbon copies
      @param subject is the subject of the message
      @param body is the boody of the message
      @param hidden if true and using KMail as the mailer, send the message
      without opening a composer window.
      @param bcc if true, send a blind carbon copy to the message sender
      @param attachment optional attachment (raw data)
      @param mailTransport defines the mail transport method. See here the
      kdepimlibs/mailtransport library.
    */
    void send( const KPIMIdentities::Identity &identity, const QString &from, const QString &to,
               const QString &cc, const QString &subject, const QString &body,
               bool hidden=false, bool bccMe=false, const QString &attachment=QString(),
               const QString &mailTransport = QString() );

  private Q_SLOTS:
    void handleQueueJobFinished( KJob* job );

  Q_SIGNALS:
    void finished( Akonadi::MailClient::Result result, const QString &errorString );

  #ifdef MAILCLIENTTEST_UNITTEST
    public:
      UnitTestResult mUnitTestResult; // So unit-tests can check the result without having to check the mail the transport sent
  #endif
};

}

Q_DECLARE_METATYPE( Akonadi::MailClient::Result );

#endif
