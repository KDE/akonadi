/*
  Copyright (c) 2002-2004 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
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

#ifndef AKONADI_FREEBUSYMANAGER_P_H
#define AKONADI_FREEBUSYMANAGER_P_H

#include "etmcalendar.h"
#include "mailscheduler_p.h"

#include <kcalcore/freebusy.h>
#include <kcalcore/icalformat.h>

#include <QtCore/QPointer>
#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusInterface>

#include <KJob>
#include <KUrl>

namespace KIO {
class Job;
}

namespace Akonadi {

class FreeBusyManager;

class FreeBusyManagerPrivate : public QObject
{
    Q_OBJECT

    FreeBusyManager *const q_ptr;
    Q_DECLARE_PUBLIC(FreeBusyManager)

public: /// Structs
    struct FreeBusyProviderRequest
    {
        FreeBusyProviderRequest(const QString &provider);

        enum Status {
            NotStarted,
            HandlingRequested,
            FreeBusyRequested
        };

        Status mRequestStatus;
        QSharedPointer<QDBusInterface> mInterface;
    };

    struct FreeBusyProvidersRequestsQueue
    {
        explicit FreeBusyProvidersRequestsQueue(const QString &start = QString(),
                                                const QString &end = QString());

        FreeBusyProvidersRequestsQueue(const KDateTime &start, const KDateTime &end);

        QString mStartTime;
        QString mEndTime;
        QList<FreeBusyProviderRequest> mRequests;
        int mHandlersCount;
        KCalCore::FreeBusy::Ptr mResultingFreeBusy;
    };

public:
    Akonadi::ETMCalendar::Ptr mCalendar;
    KCalCore::ICalFormat mFormat;

    QStringList mRetrieveQueue;
    QMap<KUrl, QString> mFreeBusyUrlEmailMap;
    QMap<QString, FreeBusyProvidersRequestsQueue> mProvidersRequestsByEmail;

    // Free/Busy uploading
    QDateTime mNextUploadTime;
    int mTimerID;
    bool mUploadingFreeBusy;
    bool mBrokenUrl;

    QPointer<QWidget > mParentWidgetForMailling;

    // the parentWidget to use while doing our "recursive" retrieval
    QPointer<QWidget>  mParentWidgetForRetrieval;

public: /// Functions
    FreeBusyManagerPrivate(FreeBusyManager *q);
    void checkFreeBusyUrl();
    QString freeBusyDir() const;
    void fetchFreeBusyUrl(const QString &email);
    QString freeBusyToIcal(const KCalCore::FreeBusy::Ptr &);
    KCalCore::FreeBusy::Ptr iCalToFreeBusy(const QByteArray &freeBusyData);
    KCalCore::FreeBusy::Ptr ownerFreeBusy();
    QString ownerFreeBusyAsString();
    void processFreeBusyDownloadResult(KJob *_job);
    void processFreeBusyUploadResult(KJob *_job);
    void uploadFreeBusy();
    QStringList getFreeBusyProviders() const;
    void queryFreeBusyProviders(const QStringList &providers, const QString &email);
    void queryFreeBusyProviders(const QStringList &providers, const QString &email,
                                const KDateTime &start, const KDateTime &end);

public Q_SLOTS:
    void processRetrieveQueue();
    void contactSearchJobFinished(KJob *_job);
    void finishProcessRetrieveQueue(const QString &email, const KUrl &url);
    void onHandlesFreeBusy(const QString &email, bool handles);
    void onFreeBusyRetrieved(const QString &email, const QString &freeBusy,
                             bool success, const QString &errorText);
    void processMailSchedulerResult(Akonadi::Scheduler::Result result, const QString &errorMsg);
    void fbCheckerJobFinished(KJob*);

Q_SIGNALS:
    void freeBusyUrlRetrieved(const QString &email, const KUrl &url);
};

class FbCheckerJob : public KJob
{
    Q_OBJECT
public:
    explicit FbCheckerJob(const QList<KUrl> &urlsToCheck, QObject *parent = 0);
    virtual void start();

    KUrl validUrl() const;

private Q_SLOTS:
    void onGetJobFinished(KJob *job);
    void dataReceived(KIO::Job *, const QByteArray &data);

private:
    void checkNextUrl();
    QList<KUrl> mUrlsToCheck;
    QByteArray mData;
    KUrl mValidUrl;
};

}

#endif // FREEBUSYMANAGER_P_H
