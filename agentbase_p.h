/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#ifndef AKONADI_AGENTBASE_P_H
#define AKONADI_AGENTBASE_P_H

#include "agentbase.h"
#include "tracerinterface.h"

#include <klocalizedstring.h>

#include <solid/networking.h>

class QSettings;
class QTimer;

namespace Akonadi {

/**
 * @internal
 */
class AgentBasePrivate : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.dfaure")

public:
    explicit AgentBasePrivate(AgentBase *parent);
    virtual ~AgentBasePrivate();
    void init();
    virtual void delayedInit();

    void slotStatus(int status, const QString &message);
    void slotPercent(int progress);
    void slotWarning(const QString &message);
    void slotError(const QString &message);
    void slotNetworkStatusChange(Solid::Networking::Status);
    void slotResumedFromSuspend();
    void slotTemporaryOfflineTimeout();

    virtual void changeProcessed();

    QString defaultReadyMessage() const
    {
        if (mOnline) {
            return i18nc("@info:status Application ready for work", "Ready");
        }
        return i18nc("@info:status", "Offline");
    }

    QString defaultSyncingMessage() const
    {
        return i18nc("@info:status", "Syncing...");
    }

    QString defaultErrorMessage() const
    {
        return i18nc("@info:status", "Error.");
    }

    QString defaultUnconfiguredMessage() const
    {
        return i18nc("@info:status", "Not configured");
    }

    void setProgramName();

    AgentBase *q_ptr;
    Q_DECLARE_PUBLIC(AgentBase)

    QString mId;
    QString mName;
    QString mResourceTypeName;

    /// Use sessionBus() to access the connection.
    QDBusConnection mDBusConnection;

    int mStatusCode;
    QString mStatusMessage;

    uint mProgress;
    QString mProgressMessage;

    bool mNeedsNetwork;
    bool mOnline;
    bool mDesiredOnlineState;

    QSettings *mSettings;

    ChangeRecorder *mChangeRecorder;

    org::freedesktop::Akonadi::Tracer *mTracer;

    AgentBase::Observer *mObserver;

    QTimer *mTemporaryOfflineTimer;

  public Q_SLOTS:
    // Dump the contents of the current ChangeReplay
    Q_SCRIPTABLE QString dumpNotificationListToString() const;
    Q_SCRIPTABLE void dumpMemoryInfo() const;
    Q_SCRIPTABLE QString dumpMemoryInfoToString() const;

    virtual void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection);
    virtual void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers);
    virtual void itemMoved(const Akonadi::Item &, const Akonadi::Collection &source, const Akonadi::Collection &destination);
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

    virtual void relationAdded(const Akonadi::Relation &relation);
    virtual void relationRemoved(const Akonadi::Relation &relation);
    virtual void itemsRelationsChanged(const Akonadi::Item::List &items,
                                       const Akonadi::Relation::List &addedRelations,
                                       const Akonadi::Relation::List &removedRelations);
};

}

#endif
