/*
    SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "notificationsubscriber.h"
#include "aggregatedfetchscope.h"
#include "akonadiserver_debug.h"
#include "localsocket_p.h"
#include "notificationmanager.h"

#include <QMimeDatabase>
#include <QPointer>

#include "private/datastream_p_p.h"
#include "private/protocol_exception_p.h"
#include "shared/akranges.h"

using namespace Akonadi;
using namespace Akonadi::Server;
using namespace AkRanges;

#define TRACE_NTF(x)
// #define TRACE_NTF(x) qCDebug(AKONADISERVER_LOG) << mSubscriber << x

NotificationSubscriber::NotificationSubscriber(NotificationManager *manager)
    : mManager(manager)
    , mSocket(nullptr)
    , mAllMonitored(false)
    , mExclusive(false)
    , mNotificationDebugging(false)
{
    if (mManager) {
        mManager->itemFetchScope()->addSubscriber();
        mManager->collectionFetchScope()->addSubscriber();
        mManager->tagFetchScope()->addSubscriber();
    }
}

NotificationSubscriber::NotificationSubscriber(NotificationManager *manager, quintptr socketDescriptor)
    : NotificationSubscriber(manager)
{
    mSocket = new LocalSocket(this);
    connect(mSocket, &QLocalSocket::readyRead, this, &NotificationSubscriber::handleIncomingData);
    connect(mSocket, &QLocalSocket::disconnected, this, &NotificationSubscriber::socketDisconnected);
    mSocket->setSocketDescriptor(socketDescriptor);

    const SchemaVersion schema = SchemaVersion::retrieveAll().at(0);

    auto hello = Protocol::HelloResponsePtr::create();
    hello->setServerName(QStringLiteral("Akonadi"));
    hello->setMessage(QStringLiteral("Not really IMAP server"));
    hello->setProtocolVersion(Protocol::version());
    hello->setGeneration(schema.generation());
    writeCommand(0, hello);
}

NotificationSubscriber::~NotificationSubscriber()
{
    QMutexLocker locker(&mLock);

    if (mNotificationDebugging) {
        Q_EMIT notificationDebuggingChanged(false);
    }
}

void NotificationSubscriber::handleIncomingData()
{
    while (mSocket->bytesAvailable() > static_cast<int>(sizeof(qint64))) {
        Protocol::DataStream stream(mSocket);

        // Ignored atm
        qint64 tag = -1;
        stream >> tag;

        Protocol::CommandPtr cmd;
        try {
            cmd = Protocol::deserialize(mSocket);
        } catch (const Akonadi::ProtocolException &e) {
            qCWarning(AKONADISERVER_LOG) << "ProtocolException while reading from notification bus for" << mSubscriber << ":" << e.what();
            disconnectSubscriber();
            return;
        } catch (const std::exception &e) {
            qCWarning(AKONADISERVER_LOG) << "Unknown exception while reading from notification bus for" << mSubscriber << ":" << e.what();
            disconnectSubscriber();
            return;
        }
        if (cmd->type() == Protocol::Command::Invalid) {
            qCWarning(AKONADISERVER_LOG) << "Invalid command while reading from notification bus for " << mSubscriber << ", resetting connection";
            disconnectSubscriber();
            return;
        }

        switch (cmd->type()) {
        case Protocol::Command::CreateSubscription:
            registerSubscriber(Protocol::cmdCast<Protocol::CreateSubscriptionCommand>(cmd));
            writeCommand(tag, Protocol::CreateSubscriptionResponsePtr::create());
            break;
        case Protocol::Command::ModifySubscription:
            if (mSubscriber.isEmpty()) {
                qCWarning(AKONADISERVER_LOG) << "Notification subscriber received ModifySubscription command before RegisterSubscriber";
                disconnectSubscriber();
                return;
            }
            modifySubscription(Protocol::cmdCast<Protocol::ModifySubscriptionCommand>(cmd));
            writeCommand(tag, Protocol::ModifySubscriptionResponsePtr::create());
            break;
        case Protocol::Command::Logout:
            disconnectSubscriber();
            break;
        default:
            qCWarning(AKONADISERVER_LOG) << "Notification subscriber for" << mSubscriber << "received an invalid command" << cmd->type();
            disconnectSubscriber();
            break;
        }
    }
}

void NotificationSubscriber::socketDisconnected()
{
    qCInfo(AKONADISERVER_LOG) << "Subscriber" << mSubscriber << "disconnected";
    disconnectSubscriber();
}

void NotificationSubscriber::disconnectSubscriber()
{
    QMutexLocker locker(&mLock);

    auto changeNtf = Protocol::SubscriptionChangeNotificationPtr::create();
    changeNtf->setSubscriber(mSubscriber);
    changeNtf->setSessionId(mSession);
    changeNtf->setOperation(Protocol::SubscriptionChangeNotification::Remove);
    mManager->slotNotify({changeNtf});

    if (mSocket) {
        disconnect(mSocket, &QLocalSocket::disconnected, this, &NotificationSubscriber::socketDisconnected);
        mSocket->close();
    }

    // Unregister ourselves from the aggregated collection fetch scope
    auto cfs = mManager->collectionFetchScope();
    cfs->apply(mCollectionFetchScope, Protocol::CollectionFetchScope());
    cfs->removeSubscriber();

    auto tfs = mManager->tagFetchScope();
    tfs->apply(mTagFetchScope, Protocol::TagFetchScope());
    tfs->removeSubscriber();

    auto ifs = mManager->itemFetchScope();
    ifs->apply(mItemFetchScope, Protocol::ItemFetchScope());
    ifs->removeSubscriber();

    mManager->forgetSubscriber(this);
    deleteLater();
}

void NotificationSubscriber::registerSubscriber(const Protocol::CreateSubscriptionCommand &command)
{
    QMutexLocker locker(&mLock);

    qCInfo(AKONADISERVER_LOG) << "Subscriber" << this << "identified as" << command.subscriberName();
    mSubscriber = command.subscriberName();
    mSession = command.session();

    auto changeNtf = Protocol::SubscriptionChangeNotificationPtr::create();
    changeNtf->setSubscriber(mSubscriber);
    changeNtf->setSessionId(mSession);
    changeNtf->setOperation(Protocol::SubscriptionChangeNotification::Add);
    mManager->slotNotify({changeNtf});
}

static QStringList canonicalMimeTypes(const QStringList &mimes)
{
    static QMimeDatabase sMimeDatabase;
    QStringList ret;
    ret.reserve(mimes.count());
    auto canonicalMime = [](const QString &mime) {
        return sMimeDatabase.mimeTypeForName(mime).name();
    };
    std::transform(mimes.begin(), mimes.end(), std::back_inserter(ret), canonicalMime);
    return ret;
}

void NotificationSubscriber::modifySubscription(const Protocol::ModifySubscriptionCommand &command)
{
    QMutexLocker locker(&mLock);

    const auto modifiedParts = command.modifiedParts();

#define START_MONITORING(type)                                                                                                                                 \
    (modifiedParts & Protocol::ModifySubscriptionCommand::ModifiedParts(Protocol::ModifySubscriptionCommand::type | Protocol::ModifySubscriptionCommand::Add))
#define STOP_MONITORING(type)                                                                                                                                  \
    (modifiedParts                                                                                                                                             \
     & Protocol::ModifySubscriptionCommand::ModifiedParts(Protocol::ModifySubscriptionCommand::type | Protocol::ModifySubscriptionCommand::Remove))

#define APPEND(set, newItemsFetch)                                                                                                                             \
    {                                                                                                                                                          \
        const auto newItems = newItemsFetch;                                                                                                                   \
        for (const auto &entity : newItems) {                                                                                                                  \
            (set).insert(entity);                                                                                                                              \
        }                                                                                                                                                      \
    }

#define REMOVE(set, itemsFetch)                                                                                                                                \
    {                                                                                                                                                          \
        const auto items = itemsFetch;                                                                                                                         \
        for (const auto &entity : items) {                                                                                                                     \
            (set).remove(entity);                                                                                                                              \
        }                                                                                                                                                      \
    }

    if (START_MONITORING(Types)) {
        APPEND(mMonitoredTypes, command.startMonitoringTypes())
    }
    if (STOP_MONITORING(Types)) {
        REMOVE(mMonitoredTypes, command.stopMonitoringTypes())
    }
    if (START_MONITORING(Collections)) {
        APPEND(mMonitoredCollections, command.startMonitoringCollections())
    }
    if (STOP_MONITORING(Collections)) {
        REMOVE(mMonitoredCollections, command.stopMonitoringCollections())
    }
    if (START_MONITORING(Items)) {
        APPEND(mMonitoredItems, command.startMonitoringItems())
    }
    if (STOP_MONITORING(Items)) {
        REMOVE(mMonitoredItems, command.stopMonitoringItems())
    }
    if (START_MONITORING(Tags)) {
        APPEND(mMonitoredTags, command.startMonitoringTags())
    }
    if (STOP_MONITORING(Tags)) {
        REMOVE(mMonitoredTags, command.stopMonitoringTags())
    }
    if (START_MONITORING(Resources)) {
        APPEND(mMonitoredResources, command.startMonitoringResources())
    }
    if (STOP_MONITORING(Resources)) {
        REMOVE(mMonitoredResources, command.stopMonitoringResources())
    }
    if (START_MONITORING(MimeTypes)) {
        APPEND(mMonitoredMimeTypes, canonicalMimeTypes(command.startMonitoringMimeTypes()))
    }
    if (STOP_MONITORING(MimeTypes)) {
        REMOVE(mMonitoredMimeTypes, canonicalMimeTypes(command.stopMonitoringMimeTypes()))
    }
    if (START_MONITORING(Sessions)) {
        APPEND(mIgnoredSessions, command.startIgnoringSessions())
    }
    if (STOP_MONITORING(Sessions)) {
        REMOVE(mIgnoredSessions, command.stopIgnoringSessions())
    }
    if (modifiedParts & Protocol::ModifySubscriptionCommand::AllFlag) {
        mAllMonitored = command.allMonitored();
    }
    if (modifiedParts & Protocol::ModifySubscriptionCommand::ExclusiveFlag) {
        mExclusive = command.isExclusive();
    }
    if (modifiedParts & Protocol::ModifySubscriptionCommand::ItemFetchScope) {
        const auto newScope = command.itemFetchScope();
        mManager->itemFetchScope()->apply(mItemFetchScope, newScope);
        mItemFetchScope = newScope;
    }
    if (modifiedParts & Protocol::ModifySubscriptionCommand::CollectionFetchScope) {
        const auto newScope = command.collectionFetchScope();
        mManager->collectionFetchScope()->apply(mCollectionFetchScope, newScope);
        mCollectionFetchScope = newScope;
    }
    if (modifiedParts & Protocol::ModifySubscriptionCommand::TagFetchScope) {
        const auto newScope = command.tagFetchScope();
        mManager->tagFetchScope()->apply(mTagFetchScope, newScope);
        mTagFetchScope = newScope;
        if (!newScope.fetchIdOnly()) {
            Q_ASSERT(!mManager->tagFetchScope()->fetchIdOnly());
        }
    }

    if (mManager) {
        if (modifiedParts & Protocol::ModifySubscriptionCommand::Types) {
            // Did the caller just subscribed to subscription changes?
            if (command.startMonitoringTypes().contains(Protocol::ModifySubscriptionCommand::SubscriptionChanges)) {
                // If yes, then send them list of all existing subscribers
                mManager->mSubscribers | Views::filter(IsNotNull) | Actions::forEach([this](const auto &subscriber) {
                    QMetaObject::invokeMethod(this,
                                              "notify",
                                              Qt::QueuedConnection,
                                              Q_ARG(Akonadi::Protocol::ChangeNotificationPtr, subscriber->toChangeNotification()));
                });
            }
            if (command.startMonitoringTypes().contains(Protocol::ModifySubscriptionCommand::ChangeNotifications)) {
                if (!mNotificationDebugging) {
                    mNotificationDebugging = true;
                    Q_EMIT notificationDebuggingChanged(true);
                }
            } else if (command.stopMonitoringTypes().contains(Protocol::ModifySubscriptionCommand::ChangeNotifications)) {
                if (mNotificationDebugging) {
                    mNotificationDebugging = false;
                    Q_EMIT notificationDebuggingChanged(false);
                }
            }
        }

        // Emit subscription change notification
        auto changeNtf = toChangeNotification();
        changeNtf->setOperation(Protocol::SubscriptionChangeNotification::Modify);
        mManager->slotNotify({changeNtf});
    }

#undef START_MONITORING
#undef STOP_MONITORING
#undef APPEND
#undef REMOVE
}

Protocol::SubscriptionChangeNotificationPtr NotificationSubscriber::toChangeNotification() const
{
    // Assumes mLock being locked by caller

    auto ntf = Protocol::SubscriptionChangeNotificationPtr::create();
    ntf->setSessionId(mSession);
    ntf->setSubscriber(mSubscriber);
    ntf->setOperation(Protocol::SubscriptionChangeNotification::Add);
    ntf->setCollections(mMonitoredCollections);
    ntf->setItems(mMonitoredItems);
    ntf->setTags(mMonitoredTags);
    ntf->setTypes(mMonitoredTypes);
    ntf->setMimeTypes(mMonitoredMimeTypes);
    ntf->setResources(mMonitoredResources);
    ntf->setIgnoredSessions(mIgnoredSessions);
    ntf->setAllMonitored(mAllMonitored);
    ntf->setExclusive(mExclusive);
    ntf->setItemFetchScope(mItemFetchScope);
    ntf->setTagFetchScope(mTagFetchScope);
    ntf->setCollectionFetchScope(mCollectionFetchScope);
    return ntf;
}

bool NotificationSubscriber::isCollectionMonitored(Entity::Id id) const
{
    // Assumes mLock being locked by caller

    if (id < 0) {
        return false;
    }

    return mMonitoredCollections.contains(id) || mMonitoredCollections.contains(0);
}

bool NotificationSubscriber::isMimeTypeMonitored(const QString &mimeType) const
{
    // Assumes mLock being locked by caller

    // KContacts::Addressee::mimeType() unfortunately uses an alias
    if (mimeType == QLatin1StringView("text/directory")) {
        return mMonitoredMimeTypes.contains(QStringLiteral("text/vcard"));
    }
    return mMonitoredMimeTypes.contains(mimeType);
}

bool NotificationSubscriber::isMoveDestinationResourceMonitored(const Protocol::ItemChangeNotification &msg) const
{
    // Assumes mLock being locked by caller

    if (msg.operation() != Protocol::ItemChangeNotification::Move) {
        return false;
    }
    return mMonitoredResources.contains(msg.destinationResource());
}

bool NotificationSubscriber::isMoveDestinationResourceMonitored(const Protocol::CollectionChangeNotification &msg) const
{
    // Assumes mLock being locked by caller

    if (msg.operation() != Protocol::CollectionChangeNotification::Move) {
        return false;
    }
    return mMonitoredResources.contains(msg.destinationResource());
}

bool NotificationSubscriber::acceptsItemNotification(const Protocol::ItemChangeNotification &msg) const
{
    // Assumes mLock being locked by caller

    if (msg.items().isEmpty()) {
        return false;
    }

    if (mAllMonitored) {
        TRACE_NTF("ACCEPTS ITEM: all monitored");
        return true;
    }

    if (!mMonitoredTypes.isEmpty() && !mMonitoredTypes.contains(Protocol::ModifySubscriptionCommand::ItemChanges)) {
        TRACE_NTF("ACCEPTS ITEM: REJECTED - Item changes not monitored");
        return false;
    }

    // we have a resource or mimetype filter
    if (!mMonitoredResources.isEmpty() || !mMonitoredMimeTypes.isEmpty()) {
        if (mMonitoredResources.contains(msg.resource())) {
            TRACE_NTF("ACCEPTS ITEM: ACCEPTED - resource monitored");
            return true;
        }

        if (isMoveDestinationResourceMonitored(msg)) {
            TRACE_NTF("ACCEPTS ITEM: ACCEPTED: move destination monitored");
            return true;
        }

        const auto items = msg.items();
        for (const auto &item : items) {
            if (isMimeTypeMonitored(item.mimeType())) {
                TRACE_NTF("ACCEPTS ITEM: ACCEPTED - mimetype monitored");
                return true;
            }
        }

        TRACE_NTF("ACCEPTS ITEM: REJECTED: resource nor mimetype monitored");
        return false;
    }

    // we explicitly monitor that item or the collections it's in
    const auto items = msg.items();
    for (const auto &item : items) {
        if (mMonitoredItems.contains(item.id())) {
            TRACE_NTF("ACCEPTS ITEM: ACCEPTED: item explicitly monitored");
            return true;
        }
    }

    if (isCollectionMonitored(msg.parentCollection())) {
        TRACE_NTF("ACCEPTS ITEM: ACCEPTED: parent collection monitored");
        return true;
    }
    if (isCollectionMonitored(msg.parentDestCollection())) {
        TRACE_NTF("ACCEPTS ITEM: ACCEPTED: destination collection monitored");
        return true;
    }

    TRACE_NTF("ACCEPTS ITEM: REJECTED");
    return false;
}

bool NotificationSubscriber::acceptsCollectionNotification(const Protocol::CollectionChangeNotification &msg) const
{
    // Assumes mLock being locked by caller

    const auto &collection = msg.collection();
    if (collection.id() < 0) {
        return false;
    }

    // HACK: We need to dispatch notifications about disabled collections to SOME
    // agents (that's what we have the exclusive subscription for) - but because
    // querying each Collection from database would be expensive, we use the
    // metadata hack to transfer this information from NotificationCollector
    if (msg.metadata().contains("DISABLED") && (msg.operation() != Protocol::CollectionChangeNotification::Unsubscribe)
        && !msg.changedParts().contains("ENABLED")) {
        // Exclusive subscriber always gets it
        // If the subscriber is not exclusive (i.e. if we got here), then the subscriber does
        // not care about this one, so drop it
        return mExclusive;
    }

    if (mAllMonitored) {
        return true;
    }

    if (!mMonitoredTypes.isEmpty() && !mMonitoredTypes.contains(Protocol::ModifySubscriptionCommand::CollectionChanges)) {
        return false;
    }

    // we have a resource filter
    if (!mMonitoredResources.isEmpty()) {
        const bool resourceMatches = mMonitoredResources.contains(msg.resource()) || isMoveDestinationResourceMonitored(msg);

        // a bit hacky, but match the behaviour from the item case,
        // if resource is the only thing we are filtering on, stop here, and if the resource filter matched, of course
        if (mMonitoredMimeTypes.isEmpty() || resourceMatches) {
            return resourceMatches;
        }
        // else continue
    }

    // we explicitly monitor that collection, or all of them
    if (isCollectionMonitored(collection.id())) {
        return true;
    }

    return isCollectionMonitored(msg.parentCollection()) || isCollectionMonitored(msg.parentDestCollection());
}

bool NotificationSubscriber::acceptsTagNotification(const Protocol::TagChangeNotification &msg) const
{
    // Assumes mLock being locked by caller

    if (msg.tag().id() < 0) {
        return false;
    }

    // Special handling for Tag removal notifications: When a Tag is removed,
    // a notification is emitted for each Resource that owns the tag (i.e.
    // each resource that owns a Tag RID - Tag RIDs are resource-specific).
    // Additionally then we send one more notification without any RID that is
    // destined for regular applications (which don't know anything about Tag RIDs)
    if (msg.operation() == Protocol::TagChangeNotification::Remove) {
        // HACK: Since have no way to determine which resource this NotificationSource
        // belongs to, we are abusing the fact that each resource ignores it's own
        // main session, which is called the same name as the resource.

        // If there are any ignored sessions, but this notification does not have
        // a specific resource set, then we ignore it, as this notification is
        // for clients, not resources (does not have tag RID)
        if (!mIgnoredSessions.isEmpty() && msg.resource().isEmpty()) {
            return false;
        }

        // If this source ignores a session (i.e. we assume it is a resource),
        // but this notification is for another resource, then we ignore it
        if (!msg.resource().isEmpty() && !mIgnoredSessions.contains(msg.resource())) {
            return false;
        }
        // Now we got here, which means that this notification either has empty
        // resource, i.e. it is destined for a client applications, or it's
        // destined for resource that we *think* (see the hack above) this
        // NotificationSource belongs too. Which means we approve this notification,
        // but it can still be discarded in the generic Tag notification filter
        // below
    }

    if (mAllMonitored) {
        return true;
    }

    if (!mMonitoredTypes.isEmpty() && !mMonitoredTypes.contains(Protocol::ModifySubscriptionCommand::TagChanges)) {
        return false;
    }

    if (mMonitoredTags.isEmpty()) {
        return true;
    }

    if (mMonitoredTags.contains(msg.tag().id())) {
        return true;
    }

    return true;
}

bool NotificationSubscriber::acceptsSubscriptionNotification(const Protocol::SubscriptionChangeNotification &msg) const
{
    // Assumes mLock being locked by caller

    Q_UNUSED(msg)

    // Unlike other types, subscription notifications must be explicitly enabled
    // by caller and are excluded from "monitor all" as well
    return mMonitoredTypes.contains(Protocol::ModifySubscriptionCommand::SubscriptionChanges);
}

bool NotificationSubscriber::acceptsDebugChangeNotification(const Protocol::DebugChangeNotification &msg) const
{
    // Assumes mLock being locked by caller

    // We should never end up sending debug notification about a debug notification.
    // This could get very messy very quickly...
    Q_ASSERT(msg.notification()->type() != Protocol::Command::DebugChangeNotification);
    if (msg.notification()->type() == Protocol::Command::DebugChangeNotification) {
        return false;
    }

    // Unlike other types, debug change notifications must be explicitly enabled
    // by caller and are excluded from "monitor all" as well
    return mMonitoredTypes.contains(Protocol::ModifySubscriptionCommand::ChangeNotifications);
}

bool NotificationSubscriber::acceptsNotification(const Protocol::ChangeNotification &msg) const
{
    // Assumes mLock being locked

    // Uninitialized subscriber gets nothing
    if (mSubscriber.isEmpty()) {
        return false;
    }

    // session is ignored
    // TODO: Should this affect SubscriptionChangeNotification and DebugChangeNotification?
    if (mIgnoredSessions.contains(msg.sessionId())) {
        return false;
    }

    switch (msg.type()) {
    case Protocol::Command::ItemChangeNotification:
        return acceptsItemNotification(static_cast<const Protocol::ItemChangeNotification &>(msg));
    case Protocol::Command::CollectionChangeNotification:
        return acceptsCollectionNotification(static_cast<const Protocol::CollectionChangeNotification &>(msg));
    case Protocol::Command::TagChangeNotification:
        return acceptsTagNotification(static_cast<const Protocol::TagChangeNotification &>(msg));
    case Protocol::Command::SubscriptionChangeNotification:
        return acceptsSubscriptionNotification(static_cast<const Protocol::SubscriptionChangeNotification &>(msg));
    case Protocol::Command::DebugChangeNotification:
        return acceptsDebugChangeNotification(static_cast<const Protocol::DebugChangeNotification &>(msg));

    default:
        qCWarning(AKONADISERVER_LOG) << "NotificationSubscriber" << mSubscriber << "received an invalid notification type" << msg.type();
        return false;
    }
}

bool NotificationSubscriber::notify(const Protocol::ChangeNotificationPtr &notification)
{
    // Guard against this object being deleted while we are waiting for the lock
    QPointer<NotificationSubscriber> ptr(this);
    QMutexLocker locker(&mLock);
    if (!ptr) {
        return false;
    }

    if (acceptsNotification(*notification)) {
        QMetaObject::invokeMethod(this, "writeNotification", Qt::QueuedConnection, Q_ARG(Akonadi::Protocol::ChangeNotificationPtr, notification));
        return true;
    }
    return false;
}

void NotificationSubscriber::writeNotification(const Protocol::ChangeNotificationPtr &notification)
{
    // tag chosen by fair dice roll
    writeCommand(4, notification);
}

void NotificationSubscriber::writeCommand(qint64 tag, const Protocol::CommandPtr &cmd)
{
    Q_ASSERT(QThread::currentThread() == thread());

    Protocol::DataStream stream(mSocket);
    stream << tag;
    try {
        Protocol::serialize(stream, cmd);
        stream.flush();
        if (!mSocket->waitForBytesWritten()) {
            if (mSocket->state() == QLocalSocket::ConnectedState) {
                qCWarning(AKONADISERVER_LOG) << "NotificationSubscriber for" << mSubscriber << ": timeout writing into stream";
            } else {
                // client has disconnected, just discard the message
            }
        }
    } catch (const ProtocolException &e) {
        qCWarning(AKONADISERVER_LOG) << "ProtocolException while writing into stream for subscriber" << mSubscriber << ":" << e.what();
    }
}

#include "moc_notificationsubscriber.cpp"
