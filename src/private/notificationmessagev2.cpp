/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#include "notificationmessagev2_p.h"
#include "notificationmessagev2_p_p.h"
#include "notificationmessage_p.h"

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusConnection>

using namespace Akonadi;


NotificationMessageV2::NotificationMessageV2()
    : d(new Private)
{
}

NotificationMessageV2::NotificationMessageV2(const NotificationMessageV2 &other)
    : d(other.d)
{
}

NotificationMessageV2::~NotificationMessageV2()
{
}

NotificationMessageV2 &NotificationMessageV2::operator=(const NotificationMessageV2 &other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

bool NotificationMessageV2::operator==(const NotificationMessageV2 &other) const
{
    return d->operation == other.d->operation
           && d->parts == other.d->parts
           && d->addedFlags == other.d->addedFlags
           && d->removedFlags == other.d->removedFlags
           && d->addedTags == other.d->addedTags
           && d->removedTags == other.d->removedTags
           && NotificationMessageHelpers::compareWithoutOpAndParts(*this, other);
}

void NotificationMessageV2::registerDBusTypes()
{
    qDBusRegisterMetaType<Akonadi::NotificationMessageV2>();
    qDBusRegisterMetaType<Akonadi::NotificationMessageV2::Entity>();
    qDBusRegisterMetaType<Akonadi::NotificationMessageV2::List>();
    qDBusRegisterMetaType<Akonadi::NotificationMessageV2::Type>();
    qDBusRegisterMetaType<QVector<QByteArray> >();
    qDBusRegisterMetaType<QVector<qint64> >();
}

bool NotificationMessageV2::isValid() const
{
    return d->operation != Akonadi::NotificationMessageV2::InvalidOp
           && d->type != Akonadi::NotificationMessageV2::InvalidType
           && !d->items.isEmpty();
}

void NotificationMessageV2::addEntity(Id id, const QString &remoteId, const QString &remoteRevision, const QString &mimeType)
{
    NotificationMessageV2::Entity item;
    item.id = id;
    item.remoteId = remoteId;
    item.remoteRevision = remoteRevision;
    item.mimeType = mimeType;

    d->items.insert(id, item);
}

void NotificationMessageV2::setEntities(const QList<NotificationMessageV2::Entity> &items)
{
    clearEntities();
    Q_FOREACH (const NotificationMessageV2::Entity &item, items) {
        d->items.insert(item.id, item);
    }
}

void NotificationMessageV2::clearEntities()
{
    d->items.clear();
}

QMap<NotificationMessageV2::Id, NotificationMessageV2::Entity> NotificationMessageV2::entities() const
{
    return d->items;
}

NotificationMessageV2::Entity NotificationMessageV2::entity(NotificationMessageV2::Id id) const
{
    return d->items.value(id);
}

QList<NotificationMessageV2::Id> NotificationMessageV2::uids() const
{
    return d->items.keys();
}

QByteArray NotificationMessageV2::sessionId() const
{
    return d->sessionId;
}

void NotificationMessageV2::setSessionId(const QByteArray &sessionId)
{
    d->sessionId = sessionId;
}

NotificationMessageV2::Type NotificationMessageV2::type() const
{
    return d->type;
}

void NotificationMessageV2::setType(Type type)
{
    d->type = type;
}

NotificationMessageV2::Operation NotificationMessageV2::operation() const
{
    return d->operation;
}

void NotificationMessageV2::setOperation(Operation operation)
{
    d->operation = operation;
}

QByteArray NotificationMessageV2::resource() const
{
    return d->resource;
}

void NotificationMessageV2::setResource(const QByteArray &resource)
{
    d->resource = resource;
}

NotificationMessageV2::Id NotificationMessageV2::parentCollection() const
{
    return d->parentCollection;
}

NotificationMessageV2::Id NotificationMessageV2::parentDestCollection() const
{
    return d->parentDestCollection;
}

void NotificationMessageV2::setParentCollection(Id parent)
{
    d->parentCollection = parent;
}

void NotificationMessageV2::setParentDestCollection(Id parent)
{
    d->parentDestCollection = parent;
}

void NotificationMessageV2::setDestinationResource(const QByteArray &destResource)
{
    d->destResource = destResource;
}

QByteArray NotificationMessageV2::destinationResource() const
{
    return d->destResource;
}

QSet<QByteArray> NotificationMessageV2::itemParts() const
{
    return d->parts;
}

void NotificationMessageV2::setItemParts(const QSet<QByteArray> &parts)
{
    d->parts = parts;
}

QSet<QByteArray> NotificationMessageV2::addedFlags() const
{
    return d->addedFlags;
}

void NotificationMessageV2::setAddedFlags(const QSet<QByteArray> &addedFlags)
{
    d->addedFlags = addedFlags;
}

QSet<QByteArray> NotificationMessageV2::removedFlags() const
{
    return d->removedFlags;
}

void NotificationMessageV2::setRemovedFlags(const QSet<QByteArray> &removedFlags)
{
    d->removedFlags = removedFlags;
}

QSet<qint64> NotificationMessageV2::addedTags() const
{
    return d->addedTags;
}

void NotificationMessageV2::setAddedTags(const QSet<qint64> &addedTags)
{
    d->addedTags = addedTags;
}

QSet<qint64> NotificationMessageV2::removedTags() const
{
    return d->removedTags;
}

void NotificationMessageV2::setRemovedTags(const QSet<qint64> &removedTags)
{
    d->removedTags = removedTags;
}

QString NotificationMessageV2::toString() const
{
    QString rv;

    switch (d->type) {
    case Items:
        rv += QLatin1String("Items ");
        break;
    case Collections:
        rv += QLatin1String("Collections ");
        break;
    case Tags:
        rv += QLatin1String("Tags ");
        break;
    case Relations:
        rv += QLatin1String("Relations ");
        break;
    case InvalidType:
        return QLatin1String("*INVALID TYPE* ");
    }

    QSet<QByteArray> items;
    Q_FOREACH (const NotificationMessageV2::Entity &item, d->items) {
        QString itemStr = QString::fromLatin1("(%1,%2").arg(item.id).arg(item.remoteId);
        if (!item.remoteRevision.isEmpty()) {
            itemStr += QString::fromLatin1(",%1").arg(item.remoteRevision);
        }
        if (!item.mimeType.isEmpty()) {
            itemStr += QString::fromLatin1(",%1").arg(item.mimeType);
        }
        itemStr += QLatin1String(")");
        items << itemStr.toLatin1();
    }
    rv += QLatin1String("(") + QString::fromLatin1(NotificationMessageHelpers::join(items, ", ")) + QLatin1String(")");

    if (d->parentDestCollection >= 0) {
        rv += QLatin1String(" from ");
    } else {
        rv += QLatin1String(" in ");
    }

    if (d->parentCollection >= 0) {
        rv += QString::fromLatin1("collection %1 ").arg(d->parentCollection);
    } else {
        rv += QLatin1String("unspecified parent collection ");
    }

    switch (d->operation) {
    case Add:
        rv += QLatin1String("added");
        break;
    case Modify:
        rv += QLatin1String("modified parts (");
        rv += QString::fromLatin1(NotificationMessageHelpers::join(d->parts, ", "));
        rv += QLatin1String(")");
        break;
    case ModifyFlags:
        rv += QLatin1String("added flags (");
        rv += QString::fromLatin1(NotificationMessageHelpers::join(d->addedFlags, ", "));
        rv += QLatin1String(") ");

        rv += QLatin1String("removed flags (");
        rv += QString::fromLatin1(NotificationMessageHelpers::join(d->removedFlags, ", "));
        rv += QLatin1String(") ");
        break;
    case ModifyTags: {
            rv += QLatin1String("added tags (");
            QList<QByteArray> tags;
            Q_FOREACH (qint64 tagId, d->addedTags) {
                tags << QByteArray::number(tagId);
            }
            rv += QString::fromLatin1(NotificationMessageHelpers::join(tags, ", "));
            rv += QLatin1String(") ");

            tags.clear();
            Q_FOREACH (qint64 tagId, d->removedTags) {
                tags << QByteArray::number(tagId);
            }
            rv += QLatin1String("removed tags (");
            rv += QString::fromLatin1(NotificationMessageHelpers::join(tags, ", "));
            rv += QLatin1String(") ");
            break;
        }
    case ModifyRelations: {
            rv += QLatin1String( "added relations (" );
            rv += QString::fromLatin1(NotificationMessageHelpers::join(d->addedFlags, ", "));
            rv += QLatin1String( ") " );

            rv += QLatin1String( "removed relations (" );
            rv += QString::fromLatin1(NotificationMessageHelpers::join(d->removedFlags, ", "));
            rv += QLatin1String( ") " );
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
        return QLatin1String("*INVALID OPERATION*");
    }

    if (d->parentDestCollection >= 0) {
        rv += QString::fromLatin1(" to collection %1").arg(d->parentDestCollection);
    }

    return rv;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Akonadi::NotificationMessageV2 &msg)
{
    arg.beginStructure();
    arg << msg.sessionId();
    arg << static_cast<int>(msg.type());
    arg << static_cast<int>(msg.operation());
    arg << msg.entities().values();
    arg << msg.resource();
    arg << msg.destinationResource();
    arg << msg.parentCollection();
    arg << msg.parentDestCollection();

    QStringList itemParts;
    Q_FOREACH (const QByteArray &itemPart, msg.itemParts()) {
        itemParts.append(QString::fromLatin1(itemPart));
    }
    arg << itemParts;

    arg << msg.addedFlags().toList();
    arg << msg.removedFlags().toList();

    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Akonadi::NotificationMessageV2 &msg)
{
    QByteArray ba;
    int i;
    QList<NotificationMessageV2::Entity> items;
    NotificationMessageV2::Id id;
    QString str;
    QStringList strl;
    QList<QByteArray> bal;

    arg.beginStructure();
    arg >> ba;
    msg.setSessionId(ba);
    arg >> i;
    msg.setType(static_cast<NotificationMessageV2::Type>(i));
    arg >> i;
    msg.setOperation(static_cast<NotificationMessageV2::Operation>(i));
    arg >> items;
    msg.setEntities(items);
    arg >> ba;
    msg.setResource(ba);
    arg >> ba;
    msg.setDestinationResource(ba);
    arg >> id;
    msg.setParentCollection(id);
    arg >> id;
    msg.setParentDestCollection(id);

    arg >> strl;

    QSet<QByteArray> itemParts;
    Q_FOREACH (const QString &itemPart, strl) {
        itemParts.insert(itemPart.toLatin1());
    }
    msg.setItemParts(itemParts);

    arg >> bal;
    msg.setAddedFlags(bal.toSet());
    arg >> bal;
    msg.setRemovedFlags(bal.toSet());

    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Akonadi::NotificationMessageV2::Entity &item)
{
    arg.beginStructure();
    arg << item.id;
    arg << item.remoteId;
    arg << item.remoteRevision;
    arg << item.mimeType;
    arg.endStructure();

    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Akonadi::NotificationMessageV2::Entity &item)
{
    arg.beginStructure();
    arg >> item.id;
    arg >> item.remoteId;
    arg >> item.remoteRevision;
    arg >> item.mimeType;
    arg.endStructure();

    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, Akonadi::NotificationMessageV2::Type type)
{
    arg.beginStructure();
    arg << static_cast<int>(type);
    arg.endStructure();

    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Akonadi::NotificationMessageV2::Type &type)
{
    int t;
    arg.beginStructure();
    arg >> t;
    arg.endStructure();
    type = static_cast<NotificationMessageV2::Type>(t);

    return arg;
}

uint qHash(const Akonadi::NotificationMessageV2 &msg)
{
    uint i = 0;
    Q_FOREACH (const NotificationMessageV2::Entity &item, msg.entities()) {
        i += item.id;
    }

    return qHash(i + (msg.type() << 31) + (msg.operation() << 28));
}

QVector<NotificationMessage> NotificationMessageV2::toNotificationV1() const
{
    QVector<NotificationMessage> v1;
    v1.reserve(d->items.count());

    Q_FOREACH (const Entity &item, d->items) {
        NotificationMessage msgv1;
        msgv1.setSessionId(d->sessionId);
        msgv1.setUid(item.id);
        msgv1.setRemoteId(item.remoteId);
        msgv1.setMimeType(item.mimeType);
        msgv1.setType(static_cast<NotificationMessage::Type>(d->type));
        if (d->operation == ModifyFlags) {
            msgv1.setOperation(NotificationMessage::Modify);
        } else {
            msgv1.setOperation(static_cast<NotificationMessage::Operation>(d->operation));
        }

        msgv1.setResource(d->resource);
        msgv1.setDestinationResource(d->destResource);
        msgv1.setParentCollection(d->parentCollection);
        msgv1.setParentDestCollection(d->parentDestCollection);

        // Backward compatibility hack
        QSet<QByteArray> parts;
        if (d->operation == Remove) {
            QByteArray rr = item.remoteRevision.toLatin1();
            parts << (rr.isEmpty() ? "1" : rr);
        } else if (d->operation == ModifyFlags) {
            parts << "FLAGS";
        } else {
            parts = d->parts;
        }
        msgv1.setItemParts(parts);

        v1 << msgv1;
    }

    return v1;
}

bool NotificationMessageV2::appendAndCompress(NotificationMessageV2::List &list, const NotificationMessageV2 &msg)
{
    return NotificationMessageHelpers::appendAndCompressImpl<NotificationMessageV2::List, NotificationMessageV2>(list, msg);
}

bool NotificationMessageV2::appendAndCompress(QList<NotificationMessageV2> &list, const NotificationMessageV2 &msg)
{
    return NotificationMessageHelpers::appendAndCompressImpl<QList<NotificationMessageV2>, NotificationMessageV2>(list, msg);
}

QDebug operator<<(QDebug dbg, const NotificationMessageV2::Entity &entity)
{
    dbg.nospace() << "(ID: " << entity.id;
    if (!entity.remoteId.isEmpty()) {
        dbg.nospace() << " RID: " << entity.remoteId;
    }
    if (!entity.remoteRevision.isEmpty()) {
        dbg.nospace() << " RREV: " << entity.remoteRevision;
    }
    if (!entity.mimeType.isEmpty()) {
        dbg.nospace() << " MimeType: " << entity.mimeType;
    }
    dbg.nospace() << ")";
    return dbg;
}
