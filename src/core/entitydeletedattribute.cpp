/*
 Copyright (c) 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

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

#include "entitydeletedattribute.h"

#include "private/imapparser_p.h"

#include "akonadicore_debug.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

using namespace Akonadi;

class EntityDeletedAttribute::EntityDeletedAttributePrivate
{
public:
    EntityDeletedAttributePrivate() {}

    Collection restoreCollection;
    QString restoreResource;
};

EntityDeletedAttribute::EntityDeletedAttribute()
    : d_ptr(new EntityDeletedAttributePrivate())
{

}

EntityDeletedAttribute::~EntityDeletedAttribute()
{
    delete d_ptr;
}

void EntityDeletedAttribute::setRestoreCollection(const Akonadi::Collection &collection)
{
    Q_D(EntityDeletedAttribute);
    if (!collection.isValid()) {
        qCWarning(AKONADICORE_LOG) << "invalid collection" << collection;
    }
    Q_ASSERT(collection.isValid());
    d->restoreCollection = collection;
    if (collection.resource().isEmpty()) {
        qCWarning(AKONADICORE_LOG) << "no resource set";
    }
    d->restoreResource = collection.resource();
}

Collection EntityDeletedAttribute::restoreCollection() const
{
    Q_D(const EntityDeletedAttribute);
    return d->restoreCollection;
}

QString EntityDeletedAttribute::restoreResource() const
{
    Q_D(const EntityDeletedAttribute);
    return d->restoreResource;
}

QByteArray Akonadi::EntityDeletedAttribute::type() const
{
    static const QByteArray sType("DELETED");
    return sType;
}

EntityDeletedAttribute *EntityDeletedAttribute::clone() const
{
    const Q_D(EntityDeletedAttribute);
    EntityDeletedAttribute *attr = new EntityDeletedAttribute();
    attr->d_ptr->restoreCollection = d->restoreCollection;
    attr->d_ptr->restoreResource = d->restoreResource;
    return attr;
}

QByteArray EntityDeletedAttribute::serialized() const
{
    const Q_D(EntityDeletedAttribute);

    QList<QByteArray> l;
    l << ImapParser::quote(d->restoreResource.toUtf8());
    QList<QByteArray> components;
    components << QByteArray::number(d->restoreCollection.id());

    l << '(' + ImapParser::join(components, " ") + ')';
    return '(' + ImapParser::join(l, " ") + ')';
}

void EntityDeletedAttribute::deserialize(const QByteArray &data)
{
    Q_D(EntityDeletedAttribute);

    QList<QByteArray> l;
    ImapParser::parseParenthesizedList(data, l);
    if (l.size() != 2) {
        qCWarning(AKONADICORE_LOG) << "invalid size";
        return;
    }
    d->restoreResource = QString::fromUtf8(l[0]);

    if (!l[1].isEmpty()) {
        QList<QByteArray> componentData;
        ImapParser::parseParenthesizedList(l[1], componentData);
        if (componentData.size() != 1) {
            return;
        }
        QList<int> components;
        bool ok;
        components << componentData.at(0).toInt(&ok);
        if (!ok) {
            return;
        }
        d->restoreCollection = Collection(components.at(0));
    }
}
