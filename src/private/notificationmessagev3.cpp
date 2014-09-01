/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#include "notificationmessagev3_p.h"
#include "notificationmessagev2_p_p.h"

#include <QDebug>
#include <QDBusMetaType>

using namespace Akonadi;

NotificationMessageV3::NotificationMessageV3()
    : NotificationMessageV2()
{
}

NotificationMessageV3::NotificationMessageV3(const NotificationMessageV3 &other)
    : NotificationMessageV2(other)
{
}

NotificationMessageV3::~NotificationMessageV3()
{
}

void NotificationMessageV3::registerDBusTypes()
{
    qDBusRegisterMetaType<Akonadi::NotificationMessageV3>();
    qDBusRegisterMetaType<Akonadi::NotificationMessageV3::List>();
}

NotificationMessageV2::List NotificationMessageV3::toV2List(const NotificationMessageV3::List &list)
{
    NotificationMessageV2::List out;
    out.reserve(list.size());
    Q_FOREACH (const NotificationMessageV3 &v3, list) {
        out << static_cast<NotificationMessageV2>(v3);
    }
    return out;
}

bool NotificationMessageV3::appendAndCompress(NotificationMessageV3::List &list, const NotificationMessageV3 &msg)
{
    return NotificationMessageHelpers::appendAndCompressImpl<NotificationMessageV3::List, NotificationMessageV3>(list, msg);
}

bool NotificationMessageV3::appendAndCompress(QList<NotificationMessageV3> &list, const NotificationMessageV3 &msg)
{
    return NotificationMessageHelpers::appendAndCompressImpl<QList<NotificationMessageV3>, NotificationMessageV3>(list, msg);
}

const QDBusArgument &operator>>(const QDBusArgument &arg, NotificationMessageV3 &msg)
{
    QByteArray ba;
    int i;
    QList<NotificationMessageV2::Entity> items;
    NotificationMessageV2::Id id;
    QString str;
    QStringList strl;
    QList<QByteArray> bal;
    QList<qint64> intl;

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
    arg >> intl;
    msg.setAddedTags(intl.toSet());
    arg >> intl;
    msg.setRemovedTags(intl.toSet());

    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const NotificationMessageV3 &msg)
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
    arg << msg.addedTags().toList();
    arg << msg.removedTags().toList();

    arg.endStructure();
    return arg;
}

QDebug operator<<(QDebug dbg, const NotificationMessageV3 &msg)
{
    dbg.nospace() << "NotificationMessageV3 {\n";
    dbg.nospace() << "\tType: ";
    switch (msg.type()) {
    case NotificationMessageV2::InvalidType:
        dbg.nospace() << QLatin1String("Invalid");
        break;
    case NotificationMessageV2::Items:
        dbg.nospace() << QLatin1String("Items");
        break;
    case NotificationMessageV2::Collections:
        dbg.nospace() << QLatin1String("Collections");
        break;
    case NotificationMessageV2::Tags:
        dbg.nospace() << QLatin1String("Tags");
        break;
    }
    dbg.nospace() << '\n';

    dbg.nospace() << "\tOperation: ";
    switch (msg.operation()) {
    case NotificationMessageV2::InvalidOp:
        dbg.nospace() << QLatin1String("Invalid");
        break;
    case NotificationMessageV2::Add:
        dbg.nospace() << QLatin1String("Add");
        break;
    case NotificationMessageV2::Modify:
        dbg.nospace() << QLatin1String("Modify");
        break;
    case NotificationMessageV2::Move:
        dbg.nospace() << QLatin1String("Move");
        break;
    case NotificationMessageV2::Remove:
        dbg.nospace() << QLatin1String("Remove");
        break;
    case NotificationMessageV2::Link:
        dbg.nospace() << QLatin1String("Link");
        break;
    case NotificationMessageV2::Unlink:
        dbg.nospace() << QLatin1String("Unlink");
        break;
    case NotificationMessageV2::Subscribe:
        dbg.nospace() << QLatin1String("Subscribe");
        break;
    case NotificationMessageV2::Unsubscribe:
        dbg.nospace() << QLatin1String("Unsubscribe");
        break;
    case NotificationMessageV2::ModifyFlags:
        dbg.nospace() << QLatin1String("ModifyFlags");
        break;
    case NotificationMessageV2::ModifyTags:
        dbg.nospace() << QLatin1String("ModifyTags");
        break;
    }
    dbg.nospace() << "\n";
    dbg.nospace() << "\tSession: " << msg.sessionId() << "\n";
    dbg.nospace() << "\tEntities: " << msg.entities().values() << "\n";
    dbg.nospace() << "\tResource: " << msg.resource() << "\n";
    dbg.nospace() << "\tCollection: " << msg.parentCollection() << "\n";
    dbg.nospace() << "\tDestination resource: " << msg.destinationResource() << "\n";
    dbg.nospace() << "\tDestination collection: " << msg.parentDestCollection() << "\n";
    dbg.nospace() << "\tParts: " << msg.itemParts() << "\n";
    dbg.nospace() << "\tAdded flags: " << msg.addedFlags() << "\n";
    dbg.nospace() << "\tRemoved flags: " << msg.removedFlags() << "\n";
    dbg.nospace() << "\tAdded tags: " << msg.addedTags() << "\n";
    dbg.nospace() << "\tRemoved tags: " << msg.removedTags() << "\n";
    dbg.nospace() << "}" << "\n";
    return dbg;
}
