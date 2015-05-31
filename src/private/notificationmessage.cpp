/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "notificationmessage_p.h"

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtDBus/QDBusMetaType>

using namespace Akonadi;

class NotificationMessage::Private : public QSharedData
{
public:
    Private()
        : QSharedData()
        , type(NotificationMessage::InvalidType)
        , operation(NotificationMessage::InvalidOp)
        , uid(-1)
        , parentCollection(-1)
        , parentDestCollection(-1)
    {
    }

    Private(const Private &other)
        : QSharedData(other)
    {
        sessionId = other.sessionId;
        type = other.type;
        operation = other.operation;
        uid = other.uid;
        remoteId = other.remoteId;
        resource = other.resource;
        destResource = other.destResource;
        parentCollection = other.parentCollection;
        parentDestCollection = other.parentDestCollection;
        mimeType = other.mimeType;
        parts = other.parts;
    }

    bool compareWithoutOpAndParts(const Private &other) const
    {
        return uid == other.uid
               && type == other.type
               && sessionId == other.sessionId
               && remoteId == other.remoteId
               && resource == other.resource
               && destResource == other.destResource
               && parentCollection == other.parentCollection
               && parentDestCollection == other.parentDestCollection
               && mimeType == other.mimeType;
    }

    bool operator==(const Private &other) const
    {
        return operation == other.operation && parts == other.parts && compareWithoutOpAndParts(other);
    }

    QByteArray sessionId;
    NotificationMessage::Type type;
    NotificationMessage::Operation operation;
    Id uid;
    QString remoteId;
    QByteArray resource;
    QByteArray destResource;
    Id parentCollection;
    Id parentDestCollection;
    QString mimeType;
    QSet<QByteArray> parts;
};

NotificationMessage::NotificationMessage()
    : d(new Private)
{
}

NotificationMessage::NotificationMessage(const NotificationMessage &other)
    : d(other.d)
{
}

NotificationMessage::~NotificationMessage()
{
}

NotificationMessage &NotificationMessage::operator=(const NotificationMessage &other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

bool NotificationMessage::operator==(const NotificationMessage &other) const
{
    return d == other.d;
}

void NotificationMessage::registerDBusTypes()
{
    qDBusRegisterMetaType<Akonadi::NotificationMessage>();
    qDBusRegisterMetaType<Akonadi::NotificationMessage::List>();
}

QByteArray NotificationMessage::sessionId() const
{
    return d->sessionId;
}

void NotificationMessage::setSessionId(const QByteArray &sessionId)
{
    d->sessionId = sessionId;
}

NotificationMessage::Type NotificationMessage::type() const
{
    return d->type;
}

void NotificationMessage::setType(Type type)
{
    d->type = type;
}

NotificationMessage::Operation NotificationMessage::operation() const
{
    return d->operation;
}

void NotificationMessage::setOperation(Operation operation)
{
    d->operation = operation;
}

NotificationMessage::Id NotificationMessage::uid() const
{
    return d->uid;
}

void NotificationMessage::setUid(Id uid)
{
    d->uid = uid;
}

QString NotificationMessage::remoteId() const
{
    return d->remoteId;
}

void NotificationMessage::setRemoteId(const QString &remoteId)
{
    d->remoteId = remoteId;
}

QByteArray NotificationMessage::resource() const
{
    return d->resource;
}

void NotificationMessage::setResource(const QByteArray &resource)
{
    d->resource = resource;
}

NotificationMessage::Id NotificationMessage::parentCollection() const
{
    return d->parentCollection;
}

NotificationMessage::Id NotificationMessage::parentDestCollection() const
{
    return d->parentDestCollection;
}

void NotificationMessage::setParentCollection(Id parent)
{
    d->parentCollection = parent;
}

void NotificationMessage::setParentDestCollection(Id parent)
{
    d->parentDestCollection = parent;
}

void NotificationMessage::setDestinationResource(const QByteArray &destResource)
{
    d->destResource = destResource;
}

QByteArray NotificationMessage::destinationResource() const
{
    return d->destResource;
}

QString NotificationMessage::mimeType() const
{
    return d->mimeType;
}

void NotificationMessage::setMimeType(const QString &mimeType)
{
    d->mimeType = mimeType;
}

QSet<QByteArray> NotificationMessage::itemParts() const
{
    return d->parts;
}

void NotificationMessage::setItemParts(const QSet<QByteArray> &parts)
{
    d->parts = parts;
}

QString NotificationMessage::toString() const
{
    QString rv;
    // some tests before making the string
    if (type() == InvalidType) {
        return QLatin1String("Error: Type is not set");
    }
    if (uid() == -1) {
        return QLatin1String("Error: uid is not set");
    }
    if (remoteId().isEmpty()) {
        return QLatin1String("Error: remoteId is empty");
    }
    if (operation() == InvalidOp) {
        return QLatin1String("Error: operation is not set");
    }

    switch (type()) {
    case Item:
        rv += QLatin1String("Item ");
        break;
    case Collection:
        rv += QLatin1String("Collection ");
        break;
    case InvalidType:
        // already done above
        break;
    }

    rv += QString::fromLatin1("(%1, %2) ").arg(uid()).arg(remoteId());

    if (parentCollection() >= 0) {
        if (parentDestCollection() >= 0) {
            rv += QString::fromLatin1("from ");
        } else {
            rv += QString::fromLatin1("in ");
        }
        rv += QString::fromLatin1("collection %1 ").arg(parentCollection());
    } else {
        rv += QLatin1String("unspecified parent collection ");
    }

    rv += QString::fromLatin1("mimetype %1 ").arg(mimeType().isEmpty() ? QLatin1String("unknown") : mimeType());

    switch (operation()) {
    case Add:
        rv += QLatin1String("added");
        break;
    case Modify: {
        rv += QLatin1String("modified parts (");
        QSet<QByteArray> parts = itemParts();
        for (auto iter = parts.cbegin(), end = parts.cend(); iter != end; ++iter) {
            if (iter != parts.cbegin()) {
                rv += QLatin1String(", ");
            }
            rv += QString::fromLatin1(*iter);
        }
        rv += QLatin1String(")");
        break;
    }
    case Move:
        rv += QLatin1String("moved");
        break;
    case Remove:
        rv += QLatin1String("removed");
        break;
    case Link:
        rv += QLatin1String("linked");
        break;
    case Unlink:
        rv += QLatin1String("unlinked");
        break;
    case Subscribe:
        rv += QLatin1String("subscribed");
        break;
    case Unsubscribe:
        rv += QLatin1String("unsubscribed");
        break;
    case InvalidOp:
        // already done above
        break;
    }

    if (parentDestCollection() >= 0) {
        rv += QString::fromLatin1(" to collection %1").arg(parentDestCollection());
    }

    return rv;
}

void NotificationMessage::appendAndCompress(NotificationMessage::List &list, const NotificationMessage &msg)
{
    bool appended;
    appendAndCompress(list, msg, &appended);
}

void NotificationMessage::appendAndCompress(NotificationMessage::List &list, const NotificationMessage &msg, bool *appended)
{
    // fast-path for stuff that is not considered during O(n) compression below
    if (msg.operation() != Add && msg.operation() != Link && msg.operation() != Unlink && msg.operation() != Subscribe && msg.operation() != Unsubscribe && msg.operation() != Move) {
        NotificationMessage::List::Iterator end = list.end();
        for (NotificationMessage::List::Iterator it = list.begin(); it != end;) {
            if (msg.d.constData()->compareWithoutOpAndParts(*((*it).d.constData()))) {
                // same operation: merge changed parts and drop the new one
                if (msg.operation() == (*it).operation()) {
                    (*it).setItemParts((*it).itemParts() + msg.itemParts());
                    *appended = false;
                    return;
                } else if (msg.operation() == Modify) {
                    // new one is a modification, the existing one not, so drop the new one
                    *appended = false;
                    return;
                } else if (msg.operation() == Remove && (*it).operation() == Modify) {
                    // new on is a deletion, erase the existing modification ones (and keep going, in case there are more)
                    it = list.erase(it);
                    end = list.end();
                } else {
                    // keep looking
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }
    *appended = true;
    list.append(msg);
}

QDBusArgument &operator<<(QDBusArgument &arg, const NotificationMessage &msg)
{
    arg.beginStructure();
    arg << msg.sessionId();
    arg << msg.type();
    arg << msg.operation();
    arg << msg.uid();
    arg << msg.remoteId();
    arg << msg.resource();
    arg << msg.parentCollection();
    arg << msg.parentDestCollection();
    arg << msg.mimeType();

    QStringList itemParts;
    if (msg.operation() == NotificationMessage::Move) {
        // encode destination resource in parts, as a backward compat hack
        itemParts.push_back(QString::fromLatin1(msg.destinationResource()));
    } else {
        Q_FOREACH (const QByteArray &itemPart, msg.itemParts()) {
            itemParts.append(QString::fromLatin1(itemPart));
        }
    }

    arg << itemParts;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, NotificationMessage &msg)
{
    arg.beginStructure();
    QByteArray b;
    arg >> b;
    msg.setSessionId(b);
    int i;
    arg >> i;
    msg.setType(static_cast<NotificationMessage::Type>(i));
    arg >> i;
    msg.setOperation(static_cast<NotificationMessage::Operation>(i));
    NotificationMessage::Id id;
    arg >> id;
    msg.setUid(id);
    QString s;
    arg >> s;
    msg.setRemoteId(s);
    arg >> b;
    msg.setResource(b);
    arg >> id;
    msg.setParentCollection(id);
    arg >> id;
    msg.setParentDestCollection(id);
    arg >> s;
    msg.setMimeType(s);
    QStringList l;
    arg >> l;

    QSet<QByteArray> itemParts;
    if (msg.operation() == NotificationMessage::Move && l.size() >= 1) {
        // decode destination resource, which is stored in parts as a backward compat hack
        msg.setDestinationResource(l.first().toLatin1());
    } else {
        Q_FOREACH (const QString &itemPart, l) {
            itemParts.insert(itemPart.toLatin1());
        }
    }

    msg.setItemParts(itemParts);
    arg.endStructure();
    return arg;
}

uint qHash(const Akonadi::NotificationMessage &msg)
{
    return qHash(msg.uid() + (msg.type() << 31) + (msg.operation() << 28));
}
