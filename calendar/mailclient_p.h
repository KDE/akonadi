/*
  Copyright (c) 1998 Barry D Benowitz <b.benowitz@telesciences.com>
  Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2009 Allen Winter <winter@kde.org>

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

#ifndef AKONADI_MAILCLIENT_P_H
#define AKONADI_MAILCLIENT_P_H

#include "akonadi-calendar_export.h"
#include "itiphandler.h"
#include <kcalcore/incidencebase.h>
#include <kmime/kmime_message.h>
#include <QObject>

struct UnitTestResult {
    typedef QList<UnitTestResult> List;
    QString from;
    QStringList to;
    QStringList cc;
    QStringList bcc;
    int transportId;
    KMime::Message::Ptr message;
    UnitTestResult() : transportId(-1) {}
};

namespace KPIMIdentities {
class Identity;
}

class KJob;

namespace Akonadi {
#ifdef PLEASE_TEST_INVITATIONS
#define EXPORT_MAILCLIENT AKONADI_CALENDAR_EXPORT
#else
#define EXPORT_MAILCLIENT
#endif

class ITIPHandlerComponentFactory;

class EXPORT_MAILCLIENT MailClient : public QObject

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

    explicit MailClient(ITIPHandlerComponentFactory *factory, QObject *parent = 0);
    ~MailClient();

    void mailAttendees(const KCalCore::IncidenceBase::Ptr &incidence,
                       const KPIMIdentities::Identity &identity,
                       bool bccMe, const QString &attachment=QString(),
                       const QString &mailTransport = QString());

    void mailOrganizer(const KCalCore::IncidenceBase::Ptr &incidence,
                       const KPIMIdentities::Identity &identity,
                       const QString &from, bool bccMe,
                       const QString &attachment=QString(),
                       const QString &sub=QString(),
                       const QString &mailTransport = QString());

    void mailTo(const KCalCore::IncidenceBase::Ptr &incidence, const KPIMIdentities::Identity &identity,
                const QString &from, bool bccMe, const QString &recipients,
                const QString &attachment = QString(), const QString &mailTransport = QString());

    /**
      Sends mail with specified from, to and subject field and body as text.
      If bcc is set, send a blind carbon copy to the sender

      @param incidence is the incidence, that is sended
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
    void send(const KCalCore::IncidenceBase::Ptr &incidence, const KPIMIdentities::Identity &identity, const QString &from, const QString &to,
              const QString &cc, const QString &subject, const QString &body,
              bool hidden=false, bool bccMe=false, const QString &attachment=QString(),
              const QString &mailTransport = QString());

private Q_SLOTS:
    void handleQueueJobFinished(KJob* job);

Q_SIGNALS:
    void finished(Akonadi::MailClient::Result result, const QString &errorString);

public:
    // For unit-test usage, since we can't depend on kdepim-runtime on jenkins
    ITIPHandlerComponentFactory *mFactory;
};

}

Q_DECLARE_METATYPE(Akonadi::MailClient::Result)

#endif