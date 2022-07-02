/*
  SPDX-FileCopyrightText: 2015 Sandro Knauß <knauss@kolabsys.com>
  SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagcache.h"
#include "akonadicore_debug.h"

#include <Akonadi/Monitor>
#include <Akonadi/TagAttribute>
#include <Akonadi/TagFetchJob>
#include <Akonadi/TagFetchScope>
#include <Akonadi/TagModifyJob>

namespace Akonadi
{
class TagCachePrivate
{
public:
    void addTag(const Akonadi::Tag &tag);
    void removeTag(const Akonadi::Tag &tag);

    QHash<Akonadi::Tag::Id, Akonadi::Tag> mCache;
    QHash<QByteArray, Akonadi::Tag::Id> mGidCache;
    QHash<QString, Akonadi::Tag::Id> mNameCache;
    Akonadi::Monitor mMonitor;
};
}

using namespace Akonadi;

TagCache::TagCache(QObject *parent)
    : QObject(parent)
    , d(new TagCachePrivate)
{
    d->mMonitor.setObjectName(QStringLiteral("TagCacheMonitor"));
    d->mMonitor.setTypeMonitored(Akonadi::Monitor::Tags);
    d->mMonitor.tagFetchScope().fetchAttribute<Akonadi::TagAttribute>();
    connect(&d->mMonitor, &Akonadi::Monitor::tagAdded, this, [this](const Akonadi::Tag &tag) {
        d->addTag(tag);
    });
    connect(&d->mMonitor, &Akonadi::Monitor::tagChanged, this, [this](const Akonadi::Tag &tag) {
        d->addTag(tag);
    });
    connect(&d->mMonitor, &Akonadi::Monitor::tagRemoved, this, [this](const Akonadi::Tag &tag) {
        d->removeTag(tag);
    });

    auto tagFetchJob = new Akonadi::TagFetchJob(this);
    tagFetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
    connect(tagFetchJob, &Akonadi::TagFetchJob::result, this, [tagFetchJob, this]() {
        if (tagFetchJob->error()) {
            qCWarning(AKONADICORE_LOG) << "Failed to fetch tags: " << tagFetchJob->errorString();
            return;
        }
        const Akonadi::Tag::List lst = tagFetchJob->tags();
        for (const Akonadi::Tag &tag : lst) {
            d->addTag(tag);
        }
    });
}

TagCache::~TagCache() = default;

Akonadi::Tag TagCache::tagByGid(const QByteArray &gid) const
{
    return d->mCache.value(d->mGidCache.value(gid));
}

Akonadi::Tag TagCache::tagByName(const QString &name) const
{
    return d->mCache.value(d->mNameCache.value(name));
}

void TagCachePrivate::addTag(const Akonadi::Tag &tag)
{
    mCache.insert(tag.id(), tag);
    mGidCache.insert(tag.gid(), tag.id());
    mNameCache.insert(tag.name(), tag.id());
}

void TagCachePrivate::removeTag(const Akonadi::Tag &tag)
{
    mCache.remove(tag.id());
    mGidCache.remove(tag.gid());
    mNameCache.remove(tag.name());
}

QColor TagCache::tagColor(const QString &tagName) const
{
    if (tagName.isEmpty()) {
        return {};
    }

    const auto tag = tagByName(tagName);
    if (const auto attr = tag.attribute<Akonadi::TagAttribute>()) {
        return attr->backgroundColor();
    }

    return {};
}

void TagCache::setTagColor(const QString &tagName, const QColor &color)
{
    Akonadi::Tag tag = tagByName(tagName);
    if (!tag.isValid()) {
        return;
    }

    auto attr = tag.attribute<Akonadi::TagAttribute>(Akonadi::Tag::AddIfMissing);
    attr->setBackgroundColor(color);
    new Akonadi::TagModifyJob(tag);
}

TagCache *TagCache::instance()
{
    static TagCache s_instance;
    return &s_instance;
}
