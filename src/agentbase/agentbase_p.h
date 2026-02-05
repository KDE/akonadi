/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2008 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "agentbase.h"
#include "tracerinterface.h"

#include <KLocalizedString>

class QSettings;
class QTimer;
class QNetworkConfigurationManager;

namespace Akonadi
{
/*!
 * \internal
 */
class AgentBasePrivate : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.dfaure")

public:
    explicit AgentBasePrivate(AgentBase *parent);
    ~AgentBasePrivate() override;
    void init();
    virtual void delayedInit();

public Q_SLOTS:
    void slotStatus(int status, const QString &message);
    void slotPercent(int progress);
    void slotWarning(const QString &message);
    void slotError(const QString &message);
    void slotNetworkStatusChange(bool isOnline);
    void slotResumedFromSuspend();
    void slotTemporaryOfflineTimeout();

    virtual void changeProcessed();

public:
    [[nodiscard]] QString defaultReadyMessage() const
    {
        if (mOnline) {
            return i18nc("@info:status Application ready for work", "Ready");
        }
        return i18nc("@info:status", "Offline");
    }

    [[nodiscard]] QString defaultSyncingMessage() const
    {
        return i18nc("@info:status", "Syncing...");
    }

    [[nodiscard]] QString defaultErrorMessage() const
    {
        return i18nc("@info:status", "Error.");
    }

    [[nodiscard]] QString defaultUnconfiguredMessage() const
    {
        return i18nc("@info:status", "Not configured");
    }

    void setProgramName();

public:
    AgentBase *q_ptr;
    Q_DECLARE_PUBLIC(AgentBase)

    QString mId;
    QString mName;
    QString mResourceTypeName;
    QStringList mActivities;

    int mStatusCode;
    QString mStatusMessage;

    int mProgress;
    QString mProgressMessage;
    QString mProgramName;

    bool mNeedsNetwork;
    bool mOnline;
    bool mDesiredOnlineState;

    bool mPendingQuit;
    bool mActivitiesEnabled = false;
    QString mAccountId;

    QSettings *mSettings = nullptr;

    ChangeRecorder *mChangeRecorder = nullptr;

    org::freedesktop::Akonadi::Tracer *mTracer = nullptr;

    AgentBase::Observer *mObserver = nullptr;
    QDBusInterface *mPowerInterface = nullptr;

    QTimer *mTemporaryOfflineTimer = nullptr;

    QEventLoopLocker *mEventLoopLocker = nullptr;
public Q_SLOTS:
    // Dump the contents of the current ChangeReplay
    Q_SCRIPTABLE QString dumpNotificationListToString() const;
    Q_SCRIPTABLE void dumpMemoryInfo() const;
    Q_SCRIPTABLE QString dumpMemoryInfoToString() const;

    virtual void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection);
    virtual void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers);
    virtual void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &source, const Akonadi::Collection &destination);
    virtual void itemRemoved(const Akonadi::Item &item);
    void itemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection);
    void itemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection);

    virtual void itemsFlagsChanged(const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags);
    virtual void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &source, const Akonadi::Collection &destination);
    virtual void itemsRemoved(const Akonadi::Item::List &items);
    virtual void itemsLinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection);
    virtual void itemsUnlinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection);

    virtual void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent);
    virtual void collectionChanged(const Akonadi::Collection &collection);
    virtual void collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &changedAttributes);
    virtual void collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &destination);
    virtual void collectionRemoved(const Akonadi::Collection &collection);
    void collectionSubscribed(const Akonadi::Collection &collection, const Akonadi::Collection &parent);
    void collectionUnsubscribed(const Akonadi::Collection &collection);

    virtual void tagAdded(const Akonadi::Tag &tag);
    virtual void tagChanged(const Akonadi::Tag &tag);
    virtual void tagRemoved(const Akonadi::Tag &tag);
    virtual void itemsTagsChanged(const Akonadi::Item::List &items, const QSet<Akonadi::Tag> &addedTags, const QSet<Akonadi::Tag> &removedTags);
};

}
