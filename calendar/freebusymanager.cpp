/*
  Copyright (c) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

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

#include "freebusymanager.h"
#include "freebusymanager_p.h"
#include "freebusydownloadjob_p.h"
#include "mailscheduler_p.h"
#include "publishdialog.h"
#include "calendarsettings.h"
#include "utils_p.h"

#include <akonadi/agentinstance.h>
#include <akonadi/agentmanager.h>
#include <akonadi/contact/contactsearchjob.h>

#include <kcalcore/event.h>
#include <kcalcore/freebusy.h>
#include <kcalcore/person.h>

#include <KDebug>
#include <KMessageBox>
#include <KStandardDirs>
#include <KTemporaryFile>
#include <KUrl>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KIO/NetAccess>
#include <KLocalizedString>

#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QTextStream>
#include <QTimer>
#include <QTimerEvent>

using namespace Akonadi;
using namespace KCalCore;

/// Free helper functions

KUrl replaceVariablesUrl(const KUrl &url, const QString &email)
{
    QString emailName;
    QString emailHost;

    const int atPos = email.indexOf('@');
    if (atPos >= 0) {
        emailName = email.left(atPos);
        emailHost = email.mid(atPos + 1);
    }

    QString saveStr = url.path();
    saveStr.replace(QRegExp("%[Ee][Mm][Aa][Ii][Ll]%"), email);
    saveStr.replace(QRegExp("%[Nn][Aa][Mm][Ee]%"), emailName);
    saveStr.replace(QRegExp("%[Ss][Ee][Rr][Vv][Ee][Rr]%"), emailHost);

    KUrl retUrl(url);
    retUrl.setPath(saveStr);
    return retUrl;
}

// We need this function because using KIO::NetAccess::exists()
// is useless for the http and https protocols. And getting back
// arbitrary data is also useless because a server can respond back
// with a "no such document" page.  So we need smart checking.
FbCheckerJob::FbCheckerJob(const QList<KUrl> &urlsToCheck, QObject *parent)
    : KJob(parent),
      mUrlsToCheck(urlsToCheck)
{
}

void FbCheckerJob::start()
{
    checkNextUrl();
}

void FbCheckerJob::checkNextUrl()
{
    if (mUrlsToCheck.isEmpty()) {
        kDebug() << "No fb file found";
        setError(KJob::UserDefinedError);
        emitResult();
        return;
    }
    const KUrl url = mUrlsToCheck.takeFirst();

    mData.clear();
    KIO::TransferJob *job = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);
    connect(job, SIGNAL(data(KIO::Job*,QByteArray)), this, SLOT(dataReceived(KIO::Job*,QByteArray)));
    connect(job, SIGNAL(result(KJob*)), this, SLOT(onGetJobFinished(KJob*)));
}

void FbCheckerJob::dataReceived(KIO::Job*, const QByteArray &data)
{
    mData.append(data);
}

void FbCheckerJob::onGetJobFinished(KJob *job)
{
    KIO::TransferJob *transferJob = static_cast<KIO::TransferJob*>(job);
    if (mData.contains("BEGIN:VCALENDAR")) {
        kDebug() << "found freebusy";
        mValidUrl = transferJob->url();
        emitResult();
    } else {
        checkNextUrl();
    }
}

KUrl FbCheckerJob::validUrl() const
{
    return mValidUrl;
}


/// FreeBusyManagerPrivate::FreeBusyProviderRequest

FreeBusyManagerPrivate::FreeBusyProviderRequest::FreeBusyProviderRequest(const QString &provider)
    : mRequestStatus(NotStarted), mInterface(0)
{
    mInterface =
        QSharedPointer<QDBusInterface>(
            new QDBusInterface("org.freedesktop.Akonadi.Resource." + provider,
                               "/FreeBusyProvider",
                               "org.freedesktop.Akonadi.Resource.FreeBusyProvider"));
}

/// FreeBusyManagerPrivate::FreeBusyProvidersRequestsQueue

FreeBusyManagerPrivate::FreeBusyProvidersRequestsQueue::FreeBusyProvidersRequestsQueue(
    const QString &start, const QString &end)
    : mHandlersCount(0), mResultingFreeBusy(0)
{
    KDateTime startDate, endDate;

    if (!start.isEmpty()) {
        mStartTime = start;
        startDate = KDateTime::fromString(start);
    } else {
        // Set the start of the period to today 00:00:00
        startDate = KDateTime(KDateTime::currentLocalDate());
        mStartTime = startDate.toString();
    }

    if (!end.isEmpty()) {
        mEndTime = end;
        endDate = KDateTime::fromString(end);
    } else {
        // Set the end of the period to today + 14 days.
        endDate = KDateTime(KDateTime::currentLocalDate()).addDays(14);
        mEndTime = endDate.toString();
    }

    mResultingFreeBusy = KCalCore::FreeBusy::Ptr(new KCalCore::FreeBusy(startDate, endDate));
}

FreeBusyManagerPrivate::FreeBusyProvidersRequestsQueue::FreeBusyProvidersRequestsQueue(
    const KDateTime &start, const KDateTime &end)
    : mHandlersCount(0), mResultingFreeBusy(0)
{
    mStartTime = start.toString();
    mEndTime = end.toString();
    mResultingFreeBusy = KCalCore::FreeBusy::Ptr(new KCalCore::FreeBusy(start, end));
}

/// FreeBusyManagerPrivate

FreeBusyManagerPrivate::FreeBusyManagerPrivate(FreeBusyManager *q)
    : QObject(),
      q_ptr(q),
      mTimerID(0),
      mUploadingFreeBusy(false),
      mBrokenUrl(false),
      mParentWidgetForRetrieval(0)
{
    connect(this, SIGNAL(freeBusyUrlRetrieved(QString,KUrl)),
            SLOT(finishProcessRetrieveQueue(QString,KUrl)));
}

QString FreeBusyManagerPrivate::freeBusyDir() const
{
    return KStandardDirs::locateLocal("data", QLatin1String("korganizer/freebusy"));
}

void FreeBusyManagerPrivate::checkFreeBusyUrl()
{
    KUrl targetURL(CalendarSettings::self()->freeBusyPublishUrl());
    mBrokenUrl = targetURL.isEmpty() || !targetURL.isValid();
}

static QString configFile()
{
    static QString file = KStandardDirs::locateLocal("data", QLatin1String("korganizer/freebusyurls"));
    return file;
}

void FreeBusyManagerPrivate::fetchFreeBusyUrl(const QString &email)
{
    // First check if there is a specific FB url for this email
    KConfig cfg(configFile());
    KConfigGroup group = cfg.group(email);
    QString url = group.readEntry(QLatin1String("url"));
    if (!url.isEmpty()) {
        kDebug() << "Found cached url:" << url;
        KUrl cachedUrl(url);
        if (Akonadi::CalendarUtils::thatIsMe(email)) {
            cachedUrl.setUser(CalendarSettings::self()->freeBusyRetrieveUser());
            cachedUrl.setPass(CalendarSettings::self()->freeBusyRetrievePassword());
        }
        emit freeBusyUrlRetrieved(email, replaceVariablesUrl(cachedUrl, email));
        return;
    }
    // Try with the url configured by preferred email in kcontactmanager
    Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob();
    job->setQuery(Akonadi::ContactSearchJob::Email, email);
    job->setProperty("contactEmail", QVariant::fromValue(email));
    connect(job, SIGNAL(result(KJob*)), this, SLOT(contactSearchJobFinished(KJob*)));
    job->start();
}

void FreeBusyManagerPrivate::contactSearchJobFinished(KJob *_job)
{
    const QString email = _job->property("contactEmail").toString();

    if (_job->error()) {
        kError() << "Error while searching for contact: "
                 << _job->errorString() << ", email = " << email;
        emit freeBusyUrlRetrieved(email, KUrl());
        return;
    }

    Akonadi::ContactSearchJob *job = qobject_cast<Akonadi::ContactSearchJob*>(_job);
    KConfig cfg(configFile());
    KConfigGroup group = cfg.group(email);
    QString url = group.readEntry(QLatin1String("url"));

    const KABC::Addressee::List contacts = job->contacts();
    foreach(const KABC::Addressee &contact, contacts) {
        const QString pref = contact.preferredEmail();
        if (!pref.isEmpty() && pref != email) {
            group = cfg.group(pref);
            url = group.readEntry("url");
            kDebug() << "Preferred email of" << email << "is" << pref;
            if (!url.isEmpty()) {
                kDebug() << "Taken url from preferred email:" << url;
                emit freeBusyUrlRetrieved(email, replaceVariablesUrl(KUrl(url), email));
                return;
            }
        }
    }
    // None found. Check if we do automatic FB retrieving then
    if (!CalendarSettings::self()->freeBusyRetrieveAuto()) {
        // No, so no FB list here
        kDebug() << "No automatic retrieving";
        emit freeBusyUrlRetrieved(email, KUrl());
        return;
    }

    // Sanity check: Don't download if it's not a correct email
    // address (this also avoids downloading for "(empty email)").
    int emailpos = email.indexOf(QLatin1Char('@'));
    if (emailpos == -1) {
        kWarning() << "No '@' found in" << email;
        emit freeBusyUrlRetrieved(email, KUrl());
        return;
    }

    const QString emailHost = email.mid(emailpos + 1);

    // Build the URL
    if (CalendarSettings::self()->freeBusyCheckHostname()) {
        // Don't try to fetch free/busy data for users not on the specified servers
        // This tests if the hostnames match, or one is a subset of the other
        const QString hostDomain = KUrl(CalendarSettings::self()->freeBusyRetrieveUrl()).host();
        if (hostDomain != emailHost &&
                !hostDomain.endsWith(QLatin1Char('.') + emailHost) &&
                !emailHost.endsWith(QLatin1Char('.') + hostDomain)) {
            // Host names do not match
            kDebug() << "Host '" << hostDomain << "' doesn't match email '" << email << '\'';
            emit freeBusyUrlRetrieved(email, KUrl());
            return;
        }
    }

    if (CalendarSettings::self()->freeBusyRetrieveUrl().contains(QRegExp("\\.[xiv]fb$"))) {
        // user specified a fullpath
        // do variable string replacements to the URL (MS Outlook style)
        const KUrl sourceUrl(CalendarSettings::self()->freeBusyRetrieveUrl());
        KUrl fullpathURL = replaceVariablesUrl(sourceUrl, email);

        // set the User and Password part of the URL
        fullpathURL.setUser(CalendarSettings::self()->freeBusyRetrieveUser());
        fullpathURL.setPass(CalendarSettings::self()->freeBusyRetrievePassword());

        // no need to cache this URL as this is pretty fast to get from the config value.
        // return the fullpath URL
        kDebug() << "Found url. email=" << email << "; url=" << fullpathURL;
        emit freeBusyUrlRetrieved(email, fullpathURL);
        return;
    }

    // else we search for a fb file in the specified URL with known possible extensions
    const QStringList extensions = QStringList() << "xfb" << "ifb" << "vfb";
    QStringList::ConstIterator ext;
    QList<KUrl> urlsToCheck;
    for (ext = extensions.constBegin(); ext != extensions.constEnd(); ++ext) {
        // build a url for this extension
        const KUrl sourceUrl = CalendarSettings::self()->freeBusyRetrieveUrl();
        KUrl dirURL = replaceVariablesUrl(sourceUrl, email);
        if (CalendarSettings::self()->freeBusyFullDomainRetrieval()) {
            dirURL.addPath(email + '.' + (*ext));
        } else {
            // Cut off everything left of the @ sign to get the user name.
            const QString emailName = email.left(emailpos);
            dirURL.addPath(emailName + '.' + (*ext));
        }
        dirURL.setUser(CalendarSettings::self()->freeBusyRetrieveUser());
        dirURL.setPass(CalendarSettings::self()->freeBusyRetrievePassword());
        urlsToCheck << dirURL;
    }
    KJob *checkerJob = new FbCheckerJob(urlsToCheck, this);
    checkerJob->setProperty("email", email);
    connect(checkerJob, SIGNAL(result(KJob*)), this, SLOT(fbCheckerJobFinished(KJob*)));
    checkerJob->start();
}

void FreeBusyManagerPrivate::fbCheckerJobFinished(KJob *job)
{
    const QString email = job->property("email").toString();
    if (!job->error()) {
        FbCheckerJob *checkerJob = static_cast<FbCheckerJob*>(job);
        KUrl dirURL = checkerJob->validUrl();
        // write the URL to the cache
        KConfig cfg(configFile());
        KConfigGroup group = cfg.group(email);
        group.writeEntry("url", dirURL.prettyUrl());   // prettyURL() does not write user nor password
        kDebug() << "Found url email=" << email << "; url=" << dirURL;
        emit freeBusyUrlRetrieved(email, dirURL);
    } else {
        kDebug() << "Returning invalid url";
        emit freeBusyUrlRetrieved(email, KUrl());
    }
}

QString FreeBusyManagerPrivate::freeBusyToIcal(const KCalCore::FreeBusy::Ptr &freebusy)
{
    return mFormat.createScheduleMessage(freebusy, KCalCore::iTIPPublish);
}

KCalCore::FreeBusy::Ptr FreeBusyManagerPrivate::iCalToFreeBusy(const QByteArray &freeBusyData)
{
    const QString freeBusyVCal(QString::fromUtf8(freeBusyData));
    KCalCore::FreeBusy::Ptr fb = mFormat.parseFreeBusy(freeBusyVCal);

    if (!fb) {
        kDebug() << "Error parsing free/busy";
        kDebug() << freeBusyVCal;
    }

    return fb;
}

KCalCore::FreeBusy::Ptr FreeBusyManagerPrivate::ownerFreeBusy()
{
    KDateTime start = KDateTime::currentUtcDateTime();
    KDateTime end = start.addDays(CalendarSettings::self()->freeBusyPublishDays());

    KCalCore::Event::List events = mCalendar ? mCalendar->rawEvents(start.date(), end.date()) : KCalCore::Event::List();
    KCalCore::FreeBusy::Ptr freebusy(new KCalCore::FreeBusy(events, start, end));
    freebusy->setOrganizer(KCalCore::Person::Ptr(
                               new KCalCore::Person(Akonadi::CalendarUtils::fullName(),
                                       Akonadi::CalendarUtils::email())));
    return freebusy;
}

QString FreeBusyManagerPrivate::ownerFreeBusyAsString()
{
    return freeBusyToIcal(ownerFreeBusy());
}

void FreeBusyManagerPrivate::processFreeBusyDownloadResult(KJob *_job)
{
    Q_Q(FreeBusyManager);

    FreeBusyDownloadJob *job = qobject_cast<FreeBusyDownloadJob *>(_job);
    Q_ASSERT(job);
    if (job->error()) {
        kError() << "Error downloading freebusy" << _job->errorString();
        KMessageBox::sorry(
            mParentWidgetForRetrieval,
            i18n("Failed to download free/busy data from: %1\nReason: %2",
                 job->url().prettyUrl(), job->errorText()),
            i18n("Free/busy retrieval error"));

        // TODO: Ask for a retry? (i.e. queue  the email again when the user wants it).

        // Make sure we don't fill up the map with unneeded data on failures.
        mFreeBusyUrlEmailMap.take(job->url());
    } else {
        KCalCore::FreeBusy::Ptr fb = iCalToFreeBusy(job->rawFreeBusyData());

        Q_ASSERT(mFreeBusyUrlEmailMap.contains(job->url()));
        const QString email = mFreeBusyUrlEmailMap.take(job->url());

        if (fb) {
            KCalCore::Person::Ptr p = fb->organizer();
            p->setEmail(email);
            q->saveFreeBusy(fb, p);
            kDebug() << "Freebusy retrieved for " << email;
            emit q->freeBusyRetrieved(fb, email);
        } else {
            kError() << "Error downloading freebusy, invalid fb.";
            KMessageBox::sorry(
                mParentWidgetForRetrieval,
                i18n("Failed to parse free/busy information that was retrieved from: %1",
                     job->url().prettyUrl()),
                i18n("Free/busy retrieval error"));
        }
    }

    // When downloading failed or finished, start a job for the next one in the
    // queue if needed.
    processRetrieveQueue();
}

void FreeBusyManagerPrivate::processFreeBusyUploadResult(KJob *_job)
{
    KIO::FileCopyJob *job = static_cast<KIO::FileCopyJob *>(_job);
    if (job->error()) {
        KMessageBox::sorry(
            job->ui()->window(),
            i18n("<qt><p>The software could not upload your free/busy list to "
                 "the URL '%1'. There might be a problem with the access "
                 "rights, or you specified an incorrect URL. The system said: "
                 "<em>%2</em>.</p>"
                 "<p>Please check the URL or contact your system administrator."
                 "</p></qt>", job->destUrl().prettyUrl(),
                 job->errorString()));
    }
    // Delete temp file
    KUrl src = job->srcUrl();
    Q_ASSERT(src.isLocalFile());
    if (src.isLocalFile()) {
        QFile::remove(src.toLocalFile());
    }
    mUploadingFreeBusy = false;
}

void FreeBusyManagerPrivate::processRetrieveQueue()
{
    if (mRetrieveQueue.isEmpty()) {
        return;
    }

    QString email = mRetrieveQueue.takeFirst();

    // First, try to find all agents that are free-busy providers
    QStringList providers = getFreeBusyProviders();
    kDebug() << "Got the following FreeBusy providers: " << providers;

    // If some free-busy providers were found let's query them first and ask them
    // if they manage the free-busy information for the email address we have.
    if (!providers.isEmpty()) {
        queryFreeBusyProviders(providers, email);
    } else {
        fetchFreeBusyUrl(email);
    }

    return;
}

void FreeBusyManagerPrivate::finishProcessRetrieveQueue(const QString &email,
        const KUrl &freeBusyUrlForEmail)
{
    Q_Q(FreeBusyManager);

    if (!freeBusyUrlForEmail.isValid()) {
        kDebug() << "Invalid FreeBusy URL" << freeBusyUrlForEmail.prettyUrl() << email;
        return;
    }

    if (mFreeBusyUrlEmailMap.contains(freeBusyUrlForEmail)) {
        kDebug() << "Download already in progress for " << freeBusyUrlForEmail;
        return;
    }

    mFreeBusyUrlEmailMap.insert(freeBusyUrlForEmail, email);

    FreeBusyDownloadJob *job = new FreeBusyDownloadJob(freeBusyUrlForEmail, mParentWidgetForRetrieval);
    q->connect(job, SIGNAL(result(KJob*)), SLOT(processFreeBusyDownloadResult(KJob*)));
    job->start();
}

void FreeBusyManagerPrivate::uploadFreeBusy()
{
    Q_Q(FreeBusyManager);

    // user has automatic uploading disabled, bail out
    if (!CalendarSettings::self()->freeBusyPublishAuto() ||
            CalendarSettings::self()->freeBusyPublishUrl().isEmpty()) {
        return;
    }

    if (mTimerID != 0) {
        // A timer is already running, so we don't need to do anything
        return;
    }

    int now = static_cast<int>(QDateTime::currentDateTime().toTime_t());
    int eta = static_cast<int>(mNextUploadTime.toTime_t()) - now;

    if (!mUploadingFreeBusy) {
        // Not currently uploading
        if (mNextUploadTime.isNull() ||
                QDateTime::currentDateTime() > mNextUploadTime) {
            // No uploading have been done in this session, or delay time is over
            q->publishFreeBusy();
            return;
        }

        // We're in the delay time and no timer is running. Start one
        if (eta <= 0) {
            // Sanity check failed - better do the upload
            q->publishFreeBusy();
            return;
        }
    } else {
        // We are currently uploading the FB list. Start the timer
        if (eta <= 0) {
            kDebug() << "This shouldn't happen! eta <= 0";
            eta = 10; // whatever
        }
    }

    // Start the timer
    mTimerID = q->startTimer(eta * 1000);

    if (mTimerID == 0) {
        // startTimer failed - better do the upload
        q->publishFreeBusy();
    }
}

QStringList FreeBusyManagerPrivate::getFreeBusyProviders() const
{
    QStringList providers;
    Akonadi::AgentInstance::List agents = Akonadi::AgentManager::self()->instances();
    foreach(const Akonadi::AgentInstance &agent, agents) {
        if (agent.type().capabilities().contains(QLatin1String("FreeBusyProvider"))) {
            providers << agent.identifier();
        }
    }
    return providers;
}

void FreeBusyManagerPrivate::queryFreeBusyProviders(const QStringList &providers,
        const QString &email)
{
    if (!mProvidersRequestsByEmail.contains(email)) {
        mProvidersRequestsByEmail[email] = FreeBusyProvidersRequestsQueue();
    }

    foreach(const QString &provider, providers) {
        FreeBusyProviderRequest request(provider);

        connect(request.mInterface.data(), SIGNAL(handlesFreeBusy(QString,bool)),
                this, SLOT(onHandlesFreeBusy(QString,bool)));

        request.mInterface->call("canHandleFreeBusy", email);
        request.mRequestStatus = FreeBusyProviderRequest::HandlingRequested;
        mProvidersRequestsByEmail[email].mRequests << request;
    }
}

void FreeBusyManagerPrivate::queryFreeBusyProviders(const QStringList &providers,
        const QString &email,
        const KDateTime &start,
        const KDateTime &end)
{
    if (!mProvidersRequestsByEmail.contains(email)) {
        mProvidersRequestsByEmail[email] = FreeBusyProvidersRequestsQueue(start, end);
    }

    queryFreeBusyProviders(providers, email);
}

void FreeBusyManagerPrivate::onHandlesFreeBusy(const QString &email, bool handles)
{
    if (!mProvidersRequestsByEmail.contains(email)) {
        return;
    }

    QDBusInterface *iface = dynamic_cast<QDBusInterface*>(sender());
    if (!iface) {
        return;
    }

    FreeBusyProvidersRequestsQueue *queue = &mProvidersRequestsByEmail[email];
    QString respondingService = iface->service();
    kDebug() << respondingService << "responded to our FreeBusy request:" << handles;
    int requestIndex = -1;

    for (int i = 0; i < queue->mRequests.size(); ++i) {
        if (queue->mRequests.at(i).mInterface->service() == respondingService) {
            requestIndex = i;
        }
    }

    if (requestIndex == -1) {
        return;
    }

    disconnect(iface, SIGNAL(handlesFreeBusy(QString,bool)),
               this, SLOT(onHandlesFreeBusy(QString,bool)));

    if (!handles) {
        queue->mRequests.removeAt(requestIndex);
        // If no more requests are left and no handler responded
        // then fall back to the URL mechanism
        if (queue->mRequests.isEmpty() && queue->mHandlersCount == 0) {
            mProvidersRequestsByEmail.remove(email);
            fetchFreeBusyUrl(email);
        }
    } else {
        ++queue->mHandlersCount;
        connect(iface, SIGNAL(freeBusyRetrieved(QString,QString,bool,QString)),
                this, SLOT(onFreeBusyRetrieved(QString,QString,bool,QString)));
        iface->call("retrieveFreeBusy", email, queue->mStartTime, queue->mEndTime);
        queue->mRequests[requestIndex].mRequestStatus = FreeBusyProviderRequest::FreeBusyRequested;
    }
}

void FreeBusyManagerPrivate::processMailSchedulerResult(Akonadi::Scheduler::Result result,
        const QString &errorMsg)
{
    if (result == Scheduler::ResultSuccess) {
        KMessageBox::information(
            mParentWidgetForMailling,
            i18n("The free/busy information was successfully sent."),
            i18n("Sending Free/Busy"),
            "FreeBusyPublishSuccess");
    } else {
        KMessageBox::error(mParentWidgetForMailling,
                           i18n("Unable to publish the free/busy data: %1", errorMsg));
    }

    sender()->deleteLater();
}

void FreeBusyManagerPrivate::onFreeBusyRetrieved(const QString &email,
        const QString &freeBusy,
        bool success,
        const QString &errorText)
{
    Q_Q(FreeBusyManager);
    Q_UNUSED(errorText);

    if (!mProvidersRequestsByEmail.contains(email)) {
        return;
    }

    QDBusInterface *iface = dynamic_cast<QDBusInterface*>(sender());
    if (!iface) {
        return;
    }

    FreeBusyProvidersRequestsQueue *queue = &mProvidersRequestsByEmail[email];
    QString respondingService = iface->service();
    int requestIndex = -1;

    for (int i = 0; i < queue->mRequests.size(); ++i) {
        if (queue->mRequests.at(i).mInterface->service() == respondingService) {
            requestIndex = i;
        }
    }

    if (requestIndex == -1) {
        return;
    }

    disconnect(iface, SIGNAL(freeBusyRetrieved(QString,QString,bool,QString)),
               this, SLOT(onFreeBusyRetrieved(QString,QString,bool,QString)));

    queue->mRequests.removeAt(requestIndex);

    if (success) {
        KCalCore::FreeBusy::Ptr fb = iCalToFreeBusy(freeBusy.toUtf8());
        if (!fb) {
            --queue->mHandlersCount;
        } else {
            queue->mResultingFreeBusy->merge(fb);
        }
    }

    if (queue->mRequests.isEmpty()) {
        if (queue->mHandlersCount == 0) {
            fetchFreeBusyUrl(email);
        } else {
            emit q->freeBusyRetrieved(queue->mResultingFreeBusy, email);
        }
        mProvidersRequestsByEmail.remove(email);
    }
}

/// FreeBusyManager::Singleton

namespace Akonadi {

class FreeBusyManagerStatic
{
public:
    FreeBusyManager instance;
};

}

K_GLOBAL_STATIC(FreeBusyManagerStatic, sManagerInstance)

FreeBusyManager::FreeBusyManager() : d_ptr(new FreeBusyManagerPrivate(this))
{
    setObjectName(QLatin1String("FreeBusyManager"));
    connect(CalendarSettings::self(), SIGNAL(configChanged()), SLOT(checkFreeBusyUrl()));
}

FreeBusyManager::~FreeBusyManager()
{
    delete d_ptr;
}

FreeBusyManager *FreeBusyManager::self()
{
    return &sManagerInstance->instance;
}

void FreeBusyManager::setCalendar(const Akonadi::ETMCalendar::Ptr &c)
{
    Q_D(FreeBusyManager);

    if (d->mCalendar) {
        disconnect(d->mCalendar.data(), SIGNAL(calendarChanged()));
    }

    d->mCalendar = c;
    if (d->mCalendar) {
        d->mFormat.setTimeSpec(d->mCalendar->timeSpec());
        connect(d->mCalendar.data(), SIGNAL(calendarChanged()), SLOT(uploadFreeBusy()));
    }

    // Lets see if we need to update our published
    QTimer::singleShot(0, this, SLOT(uploadFreeBusy()));
}

/*!
  This method is called when the user has selected to publish its
  free/busy list or when the delay have passed.
*/
void FreeBusyManager::publishFreeBusy(QWidget *parentWidget)
{
    Q_D(FreeBusyManager);
    // Already uploading? Skip this one then.
    if (d->mUploadingFreeBusy) {
        return;
    }

    // No calendar set yet? Don't upload to prevent losing published information that
    // might still be valid.
    if (!d->mCalendar) {
        return;
    }

    KUrl targetURL(CalendarSettings::self()->freeBusyPublishUrl());
    if (targetURL.isEmpty())  {
        KMessageBox::sorry(
            parentWidget,
            i18n("<qt><p>No URL configured for uploading your free/busy list. "
                 "Please set it in KOrganizer's configuration dialog, on the "
                 "\"Free/Busy\" page.</p>"
                 "<p>Contact your system administrator for the exact URL and the "
                 "account details.</p></qt>"),
            i18n("No Free/Busy Upload URL"));
        return;
    }

    if (d->mBrokenUrl) {
        // Url is invalid, don't try again
        return;
    }
    if (!targetURL.isValid()) {
        KMessageBox::sorry(
            parentWidget,
            i18n("<qt>The target URL '%1' provided is invalid.</qt>", targetURL.prettyUrl()),
            i18n("Invalid URL"));
        d->mBrokenUrl = true;
        return;
    }
    targetURL.setUser(CalendarSettings::self()->freeBusyPublishUser());
    targetURL.setPass(CalendarSettings::self()->freeBusyPublishPassword());

    d->mUploadingFreeBusy = true;

    // If we have a timer running, it should be stopped now
    if (d->mTimerID != 0) {
        killTimer(d->mTimerID);
        d->mTimerID = 0;
    }

    // Save the time of the next free/busy uploading
    d->mNextUploadTime = QDateTime::currentDateTime();
    if (CalendarSettings::self()->freeBusyPublishDelay() > 0) {
        d->mNextUploadTime =
            d->mNextUploadTime.addSecs(CalendarSettings::self()->freeBusyPublishDelay() * 60);
    }

    QString messageText = d->ownerFreeBusyAsString();

    // We need to massage the list a bit so that Outlook understands
    // it.
    messageText = messageText.replace(QRegExp(QLatin1String("ORGANIZER\\s*:MAILTO:")),
                                      QLatin1String("ORGANIZER:"));

    // Create a local temp file and save the message to it
    KTemporaryFile tempFile;
    tempFile.setAutoRemove(false);
    if (tempFile.open()) {
        QTextStream textStream(&tempFile);
        textStream << messageText;
        textStream.flush();

#if 0
        QString defaultEmail = KOCore()::self()->email();
        QString emailHost = defaultEmail.mid(defaultEmail.indexOf('@') + 1);

        // Put target string together
        KUrl targetURL;
        if (CalendarSettings::self()->publishKolab()) {
            // we use Kolab
            QString server;
            if (CalendarSettings::self()->publishKolabServer() == QLatin1String("%SERVER%") ||
                    CalendarSettings::self()->publishKolabServer().isEmpty()) {
                server = emailHost;
            } else {
                server = CalendarSettings::self()->publishKolabServer();
            }

            targetURL.setProtocol("webdavs");
            targetURL.setHost(server);

            QString fbname = CalendarSettings::self()->publishUserName();
            int at = fbname.indexOf('@');
            if (at > 1 && fbname.length() > (uint)at) {
                fbname = fbname.left(at);
            }
            targetURL.setPath("/freebusy/" + fbname + ".ifb");
            targetURL.setUser(CalendarSettings::self()->publishUserName());
            targetURL.setPass(CalendarSettings::self()->publishPassword());
        } else {
            // we use something else
            targetURL = CalendarSettings::self()->+publishAnyURL().replace("%SERVER%", emailHost);
            targetURL.setUser(CalendarSettings::self()->publishUserName());
            targetURL.setPass(CalendarSettings::self()->publishPassword());
        }
#endif

        KUrl src;
        src.setPath(tempFile.fileName());

        kDebug() << targetURL;

        KIO::Job *job = KIO::file_copy(src, targetURL, -1, KIO::Overwrite | KIO::HideProgressInfo);

        job->ui()->setWindow(parentWidget);

        connect(job, SIGNAL(result(KJob*)), SLOT(slotUploadFreeBusyResult(KJob*)));
    }
}

void FreeBusyManager::mailFreeBusy(int daysToPublish, QWidget *parentWidget)
{
    Q_D(FreeBusyManager);
    // No calendar set yet?
    if (!d->mCalendar) {
        return;
    }

    KDateTime start = KDateTime::currentUtcDateTime().toTimeSpec(d->mCalendar->timeSpec());
    KDateTime end = start.addDays(daysToPublish);

    KCalCore::Event::List events = d->mCalendar->rawEvents(start.date(), end.date());

    FreeBusy::Ptr freebusy(new FreeBusy(events, start, end));
    freebusy->setOrganizer(Person::Ptr(
                               new Person(Akonadi::CalendarUtils::fullName(),
                                          Akonadi::CalendarUtils::email())));

    QPointer<PublishDialog> publishdlg = new PublishDialog();
    if (publishdlg->exec() == QDialog::Accepted) {
        // Send the mail
        MailScheduler *scheduler = new MailScheduler(/*factory=*/0, this);
        connect(scheduler, SIGNAL(transactionFinished(Akonadi::Scheduler::Result,QString))
                , d, SLOT(processMailSchedulerResult(Akonadi::Scheduler::Result,QString)));
        d->mParentWidgetForMailling = parentWidget;

        scheduler->publish(freebusy, publishdlg->addresses());
    }
    delete publishdlg;
}

bool FreeBusyManager::retrieveFreeBusy(const QString &email, bool forceDownload,
                                       QWidget *parentWidget)
{
    Q_D(FreeBusyManager);

    kDebug() << email;
    if (email.isEmpty()) {
        kDebug() << "Email is empty";
        return false;
    }

    d->mParentWidgetForRetrieval = parentWidget;

    if (Akonadi::CalendarUtils::thatIsMe(email)) {
        // Don't download our own free-busy list from the net
        kDebug() << "freebusy of owner, not downloading";
        emit freeBusyRetrieved(d->ownerFreeBusy(), email);
        return true;
    }

    // Check for cached copy of free/busy list
    KCalCore::FreeBusy::Ptr fb = loadFreeBusy(email);
    if (fb) {
        kDebug() << "Found a cached copy for " << email;
        emit freeBusyRetrieved(fb, email);
        return true;
    }

    // Don't download free/busy if the user does not want it.
    if (!CalendarSettings::self()->freeBusyRetrieveAuto() && !forceDownload) {
        kDebug() << "Not downloading freebusy";
        return false;
    }

    d->mRetrieveQueue.append(email);

    if (d->mRetrieveQueue.count() > 1) {
        // TODO: true should always emit
        kWarning() << "Returning true without emit, is this correct?";
        return true;
    }

    // queued, because "true" means the download was initiated. So lets
    // return before starting stuff
    QMetaObject::invokeMethod(d, "processRetrieveQueue", Qt::QueuedConnection);
    return true;
}

void FreeBusyManager::cancelRetrieval()
{
    Q_D(FreeBusyManager);
    d->mRetrieveQueue.clear();
}

KCalCore::FreeBusy::Ptr FreeBusyManager::loadFreeBusy(const QString &email)
{
    Q_D(FreeBusyManager);
    const QString fbd = d->freeBusyDir();

    QFile f(fbd + QLatin1Char('/') + email + QLatin1String(".ifb"));
    if (!f.exists()) {
        kDebug() << f.fileName() << "doesn't exist.";
        return KCalCore::FreeBusy::Ptr();
    }

    if (!f.open(QIODevice::ReadOnly)) {
        kDebug() << "Unable to open file" << f.fileName();
        return KCalCore::FreeBusy::Ptr();
    }

    QTextStream ts(&f);
    QString str = ts.readAll();

    return d->iCalToFreeBusy(str.toUtf8());
}

bool FreeBusyManager::saveFreeBusy(const KCalCore::FreeBusy::Ptr &freebusy,
                                   const KCalCore::Person::Ptr &person)
{
    Q_D(FreeBusyManager);
    Q_ASSERT(person);
    kDebug() << person->fullName();

    QString fbd = d->freeBusyDir();

    QDir freeBusyDirectory(fbd);
    if (!freeBusyDirectory.exists()) {
        kDebug() << "Directory" << fbd <<" does not exist!";
        kDebug() << "Creating directory:" << fbd;

        if (!freeBusyDirectory.mkpath(fbd)) {
            kDebug() << "Could not create directory:" << fbd;
            return false;
        }
    }

    QString filename(fbd);
    filename += QLatin1Char('/');
    filename += person->email();
    filename += QLatin1String(".ifb");
    QFile f(filename);

    kDebug() << "filename:" << filename;

    freebusy->clearAttendees();
    freebusy->setOrganizer(person);

    QString messageText = d->mFormat.createScheduleMessage(freebusy, KCalCore::iTIPPublish);

    if (!f.open(QIODevice::ReadWrite)) {
        kDebug() << "acceptFreeBusy: Can't open:" << filename << "for writing";
        return false;
    }
    QTextStream t(&f);
    t << messageText;
    f.close();

    return true;
}

void FreeBusyManager::timerEvent(QTimerEvent *)
{
    publishFreeBusy();
}

#include "moc_freebusymanager.cpp"
#include "moc_freebusymanager_p.cpp"
