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

#ifndef AKONADI_NOTIFICATIONMESSAGEV2_H
#define AKONADI_NOTIFICATIONMESSAGEV2_H

#include "akonadiprotocolinternals_export.h"

#include <QtCore/QList>
#include <QtCore/QVector>
#include <QtCore/QSet>
#include <QtCore/QQueue>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QVariant>
#include <QtDBus/QDBusArgument>
#include "notificationmessage_p.h"

namespace Akonadi {

/**
  @internal
  Used for sending notification signals over DBus.
  DBus type: (ayiia(xsss)ayayxxasaayaay)
*/
class AKONADIPROTOCOLINTERNALS_EXPORT NotificationMessageV2
{
public:
    typedef QVector<NotificationMessageV2> List;
    typedef qint64 Id;

    enum Type {
        InvalidType,
        Collections,
        Items,
        Tags
    };

    // NOTE: Keep this BC with NotificationMessage - i.e. append new stuff to the end
    enum Operation {
        InvalidOp,
        Add,
        Modify,
        Move,
        Remove,
        Link,
        Unlink,
        Subscribe,
        Unsubscribe,
        ModifyFlags,
        ModifyTags
    };

    class Entity
    {
    public:
        Entity()
            : id(-1)
        {
        }

        bool operator==(const Entity &other) const
        {
            return id == other.id
                   && remoteId == other.remoteId
                   && remoteRevision == other.remoteRevision
                   && mimeType == other.mimeType;
        }

        Id id;
        QString remoteId;
        QString remoteRevision;
        QString mimeType;
    };

    NotificationMessageV2();
    NotificationMessageV2(const NotificationMessageV2 &other);
    ~NotificationMessageV2();

    NotificationMessageV2 &operator=(const NotificationMessageV2 &other);
    bool operator==(const NotificationMessageV2 &other) const;
    bool operator!=(const NotificationMessageV2 &other) const
    {
        return !operator==(other);
    }

    static void registerDBusTypes();

    bool isValid() const;

    NotificationMessageV2::Type type() const;
    void setType(NotificationMessageV2::Type type);

    NotificationMessageV2::Operation operation() const;
    void setOperation(NotificationMessageV2::Operation operation);

    QByteArray sessionId() const;
    void setSessionId(const QByteArray &session);

    void addEntity(Id id, const QString &remoteId = QString(), const QString &remoteRevision = QString(), const QString &mimeType = QString());
    void setEntities(const QList<NotificationMessageV2::Entity> &items);
    QMap<Id, NotificationMessageV2::Entity> entities() const;
    NotificationMessageV2::Entity entity(Id id) const;
    QList<Id> uids() const;
    void clearEntities();

    QByteArray resource() const;
    void setResource(const QByteArray &resource);

    Id parentCollection() const;
    void setParentCollection(Id parent);

    Id parentDestCollection() const;
    void setParentDestCollection(Id parent);

    QByteArray destinationResource() const;
    void setDestinationResource(const QByteArray &destResource);

    QSet<QByteArray> itemParts() const;
    void setItemParts(const QSet<QByteArray> &parts);

    QSet<QByteArray> addedFlags() const;
    void setAddedFlags(const QSet<QByteArray> &parts);

    QSet<QByteArray> removedFlags() const;
    void setRemovedFlags(const QSet<QByteArray> &parts);

    QSet<qint64> addedTags() const;
    void setAddedTags(const QSet<qint64> &tags);

    QSet<qint64> removedTags() const;
    void setRemovedTags(const QSet<qint64> &tags);

    QString toString() const;

    QVector<NotificationMessage> toNotificationV1() const;

    static bool appendAndCompress(NotificationMessageV2::List &list, const NotificationMessageV2 &msg);
    static bool appendAndCompress(QList<NotificationMessageV2> &list, const NotificationMessageV2 &msg);

protected:
    class Private;
    QSharedDataPointer<Private> d;
};

} // namespace Akonadi

AKONADIPROTOCOLINTERNALS_EXPORT QDebug operator<<(QDebug debug, const Akonadi::NotificationMessageV2::Entity &entity);

const QDBusArgument &operator>>(const QDBusArgument &arg, Akonadi::NotificationMessageV2 &msg);
QDBusArgument &operator<<(QDBusArgument &arg, const Akonadi::NotificationMessageV2 &msg);
const QDBusArgument &operator>>(const QDBusArgument &arg, Akonadi::NotificationMessageV2::Entity &item);
QDBusArgument &operator<<(QDBusArgument &arg, const Akonadi::NotificationMessageV2::Entity &item);
const QDBusArgument &operator>>(const QDBusArgument &arg, Akonadi::NotificationMessageV2::Type &type);
QDBusArgument &operator<<(QDBusArgument &qrg, Akonadi::NotificationMessageV2::Type type);
uint qHash(const Akonadi::NotificationMessageV2 &msg);

Q_DECLARE_TYPEINFO(Akonadi::NotificationMessageV2, Q_MOVABLE_TYPE);

Q_DECLARE_METATYPE(Akonadi::NotificationMessageV2)
Q_DECLARE_METATYPE(Akonadi::NotificationMessageV2::Entity)
Q_DECLARE_METATYPE(Akonadi::NotificationMessageV2::List)
Q_DECLARE_METATYPE(Akonadi::NotificationMessageV2::Type)
Q_DECLARE_METATYPE(QVector<Akonadi::NotificationMessageV2::Type>)
Q_DECLARE_METATYPE(QVector<QByteArray>)
Q_DECLARE_METATYPE(QVector<qint64>)

// V3 is used in NotificationSource.xml interface, so it must be
// defined so that old clients that only include this header
// will compile
#include "notificationmessagev3_p.h"

#endif // NOTIFICATIONMESSAGEV2_H
