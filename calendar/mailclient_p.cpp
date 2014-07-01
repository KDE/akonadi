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

#include "mailclient_p.h"

#include "akonadi/kdepimlibs-version.h"

#include <akonadi/collection.h>

#include <kcalcore/attendee.h>
#include <kcalcore/incidence.h>
#include <kcalcore/incidencebase.h>
#include <kcalutils/incidenceformatter.h>
#include <kmime/kmime_message.h>
#include <kpimidentities/identity.h>
#include <kpimutils/email.h>

#include <mailtransport/messagequeuejob.h>
#include <mailtransport/transport.h>
#include <mailtransport/transportmanager.h>

#include <KDebug>
#include <KLocalizedString>
#include <KJob>
#include <KProtocolManager>
#include <KSystemTimeZone>

using namespace Akonadi;


MailClient::MailClient(ITIPHandlerComponentFactory *factory, QObject *parent) : QObject(parent)
    , mFactory(factory ? factory : new ITIPHandlerComponentFactory(this))
{
}

MailClient::~MailClient()
{
}

void MailClient::mailAttendees(const KCalCore::IncidenceBase::Ptr &incidence,
                               const KPIMIdentities::Identity &identity,
                               bool bccMe, const QString &attachment,
                               const QString &mailTransport)
{
    Q_ASSERT(incidence);
    KCalCore::Attendee::List attendees = incidence->attendees();
    if (attendees.isEmpty()) {
        kWarning() << "There are no attendees to e-mail";
        emit finished(ResultNoAttendees, i18n("There are no attendees to e-mail"));
        return;
    }

    const QString from = incidence->organizer()->fullName();
    const QString organizerEmail = incidence->organizer()->email();

    QStringList toList;
    QStringList ccList;
    const int numberOfAttendees = attendees.count();
    for (int i=0; i<numberOfAttendees; ++i) {
        KCalCore::Attendee::Ptr a = attendees.at(i);

        const QString email = a->email();
        if (email.isEmpty()) {
            continue;
        }

        // In case we (as one of our identities) are the organizer we are sending
        // this mail. We could also have added ourselves as an attendee, in which
        // case we don't want to send ourselves a notification mail.
        if (organizerEmail == email) {
            continue;
        }

        // Optional Participants and Non-Participants are copied on the email
        if (a->role() == KCalCore::Attendee::OptParticipant ||
                a->role() == KCalCore::Attendee::NonParticipant) {
            ccList << a->fullName();
        } else {
            toList << a->fullName();
        }
    }
    if (toList.isEmpty() && ccList.isEmpty()) {
        // Not really to be called a groupware meeting, eh
        kWarning() << "There are really no attendees to e-mail";
        emit finished(ResultReallyNoAttendees, i18n("There are no attendees to e-mail"));
        return;
    }
    QString to;
    if (!toList.isEmpty()) {
        to = toList.join(QLatin1String(", "));
    }
    QString cc;
    if (!ccList.isEmpty()) {
        cc = ccList.join(QLatin1String(", "));
    }

    QString subject;
    if (incidence->type() != KCalCore::Incidence::TypeFreeBusy) {
        KCalCore::Incidence::Ptr inc = incidence.staticCast<KCalCore::Incidence>();
        subject = inc->summary();
    } else {
        subject = i18n("Free Busy Object");
    }

    const QString body = KCalUtils::IncidenceFormatter::mailBodyStr(incidence,
                         KSystemTimeZones::local());

    send(incidence, identity, from, to, cc, subject, body, false, bccMe, attachment, mailTransport);
}

void MailClient::mailOrganizer(const KCalCore::IncidenceBase::Ptr &incidence,
                               const KPIMIdentities::Identity &identity,
                               const QString &from, bool bccMe,
                               const QString &attachment,
                               const QString &sub, const QString &mailTransport)
{
    const QString to = incidence->organizer()->fullName();
    QString subject = sub;

    if (incidence->type() != KCalCore::Incidence::TypeFreeBusy) {
        KCalCore::Incidence::Ptr inc = incidence.staticCast<KCalCore::Incidence>();
        if (subject.isEmpty()) {
            subject = inc->summary();
        }
    } else {
        subject = i18n("Free Busy Message");
    }

    const QString body = KCalUtils::IncidenceFormatter::mailBodyStr(incidence,
                         KSystemTimeZones::local());

    send(incidence, identity, from, to, QString(), subject, body, false, bccMe, attachment, mailTransport);
}

void MailClient::mailTo(const KCalCore::IncidenceBase::Ptr &incidence,
                        const KPIMIdentities::Identity &identity,
                        const QString &from, bool bccMe,
                        const QString &recipients, const QString &attachment,
                        const QString &mailTransport)
{
    QString subject;

    if (incidence->type() != KCalCore::Incidence::TypeFreeBusy) {
        KCalCore::Incidence::Ptr inc = incidence.staticCast<KCalCore::Incidence>() ;
        subject = inc->summary();
    } else {
        subject = i18n("Free Busy Message");
    }

    const QString body = KCalUtils::IncidenceFormatter::mailBodyStr(incidence,
                         KSystemTimeZones::local());

    send(incidence, identity, from, recipients, QString(), subject, body, false,
         bccMe, attachment, mailTransport);
}

static QStringList extractEmailAndNormalize(const QString &email)
{
    const QStringList splittedEmail = KPIMUtils::splitAddressList(email);
    QStringList normalizedEmail;
    foreach(const QString &email, splittedEmail) {
        const QString str = KPIMUtils::extractEmailAddress(KPIMUtils::normalizeAddressesAndEncodeIdn(email));
        normalizedEmail << str;
    }
    return normalizedEmail;
}

void MailClient::send(const KCalCore::IncidenceBase::Ptr &incidence,
                      const KPIMIdentities::Identity &identity,
                      const QString &from, const QString &_to,
                      const QString &cc, const QString &subject,
                      const QString &body, bool hidden, bool bccMe,
                      const QString &attachment, const QString &mailTransport)
{
    Q_UNUSED(hidden);

    UnitTestResult unitTestResult;

    if (!MailTransport::TransportManager::self()->showTransportCreationDialog(
                0, MailTransport::TransportManager::IfNoTransportExists)) {
        kError() << "Error while creating transport";
        emit finished(ResultErrorCreatingTransport, i18n("Error while creating transport"));
        return;
    }

    // We must have a recipients list for most MUAs. Thus, if the 'to' list
    // is empty simply use the 'from' address as the recipient.
    QString to = _to;
    if (to.isEmpty()) {
        to = from;
    }
    kDebug() << "\nFrom:" << from
             << "\nTo:" << to
             << "\nCC:" << cc
             << "\nSubject:" << subject << "\nBody: \n" << body
             << "\nAttachment:\n" << attachment
             << "\nmailTransport: " << mailTransport;

    MailTransport::Transport *transport =
        MailTransport::TransportManager::self()->transportByName(mailTransport);

    if (!transport) {
        transport =
            MailTransport::TransportManager::self()->transportByName(
                MailTransport::TransportManager::self()->defaultTransportName());
    }

    if (!transport) {
        kError() << "Error fetching transport; mailTransport"
                 << mailTransport << MailTransport::TransportManager::self()->defaultTransportName();
        emit finished(ResultErrorFetchingTransport,
                      i18n("Error fetching transport. Unable to send invitations"));
        return;
    }

    const int transportId = transport->id();

    // gather config values
    KConfig config(QLatin1String("mailviewerrc"));

    KConfigGroup configGroup(&config, QLatin1String("Invitations"));
    const bool outlookConformInvitation = configGroup.readEntry("LegacyBodyInvites",
#ifdef KDEPIM_ENTERPRISE_BUILD
                                          true
#else
                                          false
#endif
                                                               );

    // Now build the message we like to send. The message KMime::Message::Ptr instance
    // will be the root message that has 2 additional message. The body itself and
    // the attached cal.ics calendar file.
    KMime::Message::Ptr message = KMime::Message::Ptr(new KMime::Message);
    message->contentTransferEncoding()->clear();  // 7Bit, decoded.

    // Set the headers

    message->userAgent()->fromUnicodeString(
        KProtocolManager::userAgentForApplication(
            QLatin1String("KOrganizer"), QLatin1String(KDEPIMLIBS_VERSION)), "utf-8");

    message->from()->fromUnicodeString(from, "utf-8");
    message->to()->fromUnicodeString(to, "utf-8");
    message->cc()->fromUnicodeString(cc, "utf-8");
    if (bccMe) {
        message->bcc()->fromUnicodeString(from, "utf-8");   //from==me, right?
    }
    message->date()->setDateTime(KDateTime::currentLocalDateTime());
    message->subject()->fromUnicodeString(subject, "utf-8");

    if (outlookConformInvitation) {
        message->contentType()->setMimeType("text/calendar");
        message->contentType()->setCharset("utf-8");
        message->contentType()->setName(QLatin1String("cal.ics"), "utf-8");
        message->contentType()->setParameter(QLatin1String("method"), QLatin1String("request"));

        if (!attachment.isEmpty()) {
            KMime::Headers::ContentDisposition *disposition =
                new KMime::Headers::ContentDisposition(message.get());
            disposition->setDisposition(KMime::Headers::CDinline);
            message->setHeader(disposition);
            message->contentTransferEncoding()->setEncoding(KMime::Headers::CEquPr);
            message->setBody(KMime::CRLFtoLF(attachment.toUtf8()));
        }
    } else {
        // We need to set following 4 lines by hand else KMime::Content::addContent
        // will create a new Content instance for us to attach the main message
        // what we don't need cause we already have the main message instance where
        // 2 additional messages are attached.
        KMime::Headers::ContentType *ct = message->contentType();
        ct->setMimeType("multipart/mixed");
        ct->setBoundary(KMime::multiPartBoundary());
        ct->setCategory(KMime::Headers::CCcontainer);

        // Set the first multipart, the body message.
        KMime::Content *bodyMessage = new KMime::Content;
        KMime::Headers::ContentDisposition *bodyDisposition =
            new KMime::Headers::ContentDisposition(bodyMessage);
        bodyDisposition->setDisposition(KMime::Headers::CDinline);
        bodyMessage->contentType()->setMimeType("text/plain");
        bodyMessage->contentType()->setCharset("utf-8");
        bodyMessage->contentTransferEncoding()->setEncoding(KMime::Headers::CEquPr);
        bodyMessage->setBody(KMime::CRLFtoLF(body.toUtf8()));
        message->addContent(bodyMessage);

        // Set the sedcond multipart, the attachment.
        if (!attachment.isEmpty()) {
            KMime::Content *attachMessage = new KMime::Content;
            KMime::Headers::ContentDisposition *attachDisposition =
                new KMime::Headers::ContentDisposition(attachMessage);
            attachDisposition->setDisposition(KMime::Headers::CDattachment);
            attachMessage->contentType()->setMimeType("text/calendar");
            attachMessage->contentType()->setCharset("utf-8");
            attachMessage->contentType()->setName(QLatin1String("cal.ics"), "utf-8");
            attachMessage->contentType()->setParameter(QLatin1String("method"),
                    QLatin1String("request"));
            attachMessage->setHeader(attachDisposition);
            attachMessage->contentTransferEncoding()->setEncoding(KMime::Headers::CEquPr);
            attachMessage->setBody(KMime::CRLFtoLF(attachment.toUtf8()));
            message->addContent(attachMessage);
        }
    }

    // Job done, attach the both multiparts and assemble the message.
    message->assemble();

    // Put the newly created item in the MessageQueueJob.
    MailTransport::MessageQueueJob *qjob = mFactory->createMessageQueueJob(incidence, identity, this);
    qjob->transportAttribute().setTransportId(transportId);
    qjob->sentBehaviourAttribute().setSentBehaviour(
        MailTransport::SentBehaviourAttribute::MoveToDefaultSentCollection);

    const QString unormalizedFrom = (transport && transport->specifySenderOverwriteAddress()) ?
                                    transport->senderOverwriteAddress() : from;

    const QString normalizedFrom = KPIMUtils::extractEmailAddress(
                                       KPIMUtils::normalizeAddressesAndEncodeIdn(unormalizedFrom));

    const QString finalFrom = KPIMUtils::extractEmailAddress(normalizedFrom);
    qjob->addressAttribute().setFrom(finalFrom);

    QStringList toStringList;
    if (!to.isEmpty()) {
        toStringList = extractEmailAndNormalize(to);
        qjob->addressAttribute().setTo(toStringList);
    }

    QStringList ccStringList;
    if (!cc.isEmpty()) {
        ccStringList = extractEmailAndNormalize(cc);
        qjob->addressAttribute().setCc(ccStringList);
    }

    QStringList bccStringList;
    if (bccMe) {
        bccStringList = extractEmailAndNormalize(from);
        qjob->addressAttribute().setBcc(bccStringList);
    }

    qjob->setMessage(message);
    connect(qjob, SIGNAL(finished(KJob*)), SLOT(handleQueueJobFinished(KJob*)));
    qjob->start();
}

void MailClient::handleQueueJobFinished(KJob *job)
{
    if (job->error()) {
        kError() << "Error queueing message:" << job->errorText();
        emit finished(ResultQueueJobError, i18n("Error queuing message in outbox: %1",
                                                job->errorText()));
    } else {
        emit finished(ResultSuccess, QString());
    }
}
