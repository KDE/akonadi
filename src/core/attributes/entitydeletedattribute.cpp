/*
 SPDX-FileCopyrightText: 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

 SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "entitydeletedattribute.h"

#include "private/imapparser_p.h"

#include "akonadicore_debug.h"

#include <QByteArray>
#include <QString>

using namespace Akonadi;

class EntityDeletedAttribute::EntityDeletedAttributePrivate
{
public:
    Collection restoreCollection;
    QString restoreResource;
};

EntityDeletedAttribute::EntityDeletedAttribute()
    : d(std::make_unique<EntityDeletedAttributePrivate>())
{

}

EntityDeletedAttribute::~EntityDeletedAttribute() = default;

void EntityDeletedAttribute::setRestoreCollection(const Akonadi::Collection &collection)
{
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
    return d->restoreCollection;
}

QString EntityDeletedAttribute::restoreResource() const
{
    return d->restoreResource;
}

QByteArray Akonadi::EntityDeletedAttribute::type() const
{
    return QByteArrayLiteral("DELETED");
}

EntityDeletedAttribute *EntityDeletedAttribute::clone() const
{
    auto *attr = new EntityDeletedAttribute();
    attr->d->restoreCollection = d->restoreCollection;
    attr->d->restoreResource = d->restoreResource;
    return attr;
}

QByteArray EntityDeletedAttribute::serialized() const
{
    QList<QByteArray> l;
    l << ImapParser::quote(d->restoreResource.toUtf8());
    QList<QByteArray> components;
    components << QByteArray::number(d->restoreCollection.id());

    l << '(' + ImapParser::join(components, " ") + ')';
    return '(' + ImapParser::join(l, " ") + ')';
}

void EntityDeletedAttribute::deserialize(const QByteArray &data)
{
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
        bool ok;
        const int components = componentData.at(0).toInt(&ok);
        if (!ok) {
            return;
        }
        d->restoreCollection = Collection(components);
    }
}
