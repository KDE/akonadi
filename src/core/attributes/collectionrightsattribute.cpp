/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionrightsattribute_p.h"

using namespace Akonadi;

static Collection::Rights dataToRights(const QByteArray &data)
{
    Collection::Rights rights = Collection::ReadOnly;

    if (data.isEmpty()) {
        return Collection::ReadOnly;
    }

    if (data.at(0) == 'a') {
        return Collection::AllRights;
    }

    for (int i = 0; i < data.size(); ++i) {
        switch (data.at(i)) {
        case 'w':
            rights |= Collection::CanChangeItem;
            break;
        case 'c':
            rights |= Collection::CanCreateItem;
            break;
        case 'd':
            rights |= Collection::CanDeleteItem;
            break;
        case 'l':
            rights |= Collection::CanLinkItem;
            break;
        case 'u':
            rights |= Collection::CanUnlinkItem;
            break;
        case 'W':
            rights |= Collection::CanChangeCollection;
            break;
        case 'C':
            rights |= Collection::CanCreateCollection;
            break;
        case 'D':
            rights |= Collection::CanDeleteCollection;
            break;
        }
    }

    return rights;
}

static QByteArray rightsToData(Collection::Rights &rights)
{
    if (rights == Collection::AllRights) {
        return QByteArray("a");
    }

    QByteArray data;
    if (rights & Collection::CanChangeItem) {
        data.append('w');
    }
    if (rights & Collection::CanCreateItem) {
        data.append('c');
    }
    if (rights & Collection::CanDeleteItem) {
        data.append('d');
    }
    if (rights & Collection::CanChangeCollection) {
        data.append('W');
    }
    if (rights & Collection::CanCreateCollection) {
        data.append('C');
    }
    if (rights & Collection::CanDeleteCollection) {
        data.append('D');
    }
    if (rights & Collection::CanLinkItem) {
        data.append('l');
    }
    if (rights & Collection::CanUnlinkItem) {
        data.append('u');
    }

    return data;
}

/**
 * @internal
 */
class Akonadi::CollectionRightsAttributePrivate
{
public:
    QByteArray mData;
};

CollectionRightsAttribute::CollectionRightsAttribute()
    : d(new CollectionRightsAttributePrivate())
{
}

CollectionRightsAttribute::~CollectionRightsAttribute() = default;

void CollectionRightsAttribute::setRights(Collection::Rights rights)
{
    d->mData = rightsToData(rights);
}

Collection::Rights CollectionRightsAttribute::rights() const
{
    return dataToRights(d->mData);
}

CollectionRightsAttribute *CollectionRightsAttribute::clone() const
{
    auto attr = new CollectionRightsAttribute();
    attr->d->mData = d->mData;

    return attr;
}

QByteArray CollectionRightsAttribute::type() const
{
    static const QByteArray s_accessRightsIdentifier("AccessRights");
    return s_accessRightsIdentifier;
}

QByteArray CollectionRightsAttribute::serialized() const
{
    return d->mData;
}

void CollectionRightsAttribute::deserialize(const QByteArray &data)
{
    d->mData = data;
}
