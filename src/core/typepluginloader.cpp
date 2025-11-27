/*
    SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "typepluginloader_p.h"

#include "item.h"
#include "itemserializer_p.h"
#include "itemserializerplugin.h"

#include "akonadicore_debug.h"

// Qt
#include <QByteArray>
#include <QHash>
#include <QMimeDatabase>
#include <QMimeType>
#include <QRegularExpression>
#include <QStack>
#include <QString>
#include <QStringList>

// temporary
#include "pluginloader_p.h"

#include <cassert>
#include <vector>

static const char LEGACY_NAME[] = "legacy";
static const char DEFAULT_NAME[] = "default";
static const char _APPLICATION_OCTETSTREAM[] = "application/octet-stream";

namespace Akonadi
{
Q_GLOBAL_STATIC(DefaultItemSerializerPlugin, s_defaultItemSerializerPlugin) // NOLINT(readability-redundant-member-init)

class PluginEntry
{
public:
    PluginEntry()
        : mPlugin(nullptr)
    {
    }

    explicit PluginEntry(const QString &identifier, QObject *plugin = nullptr)
        : mIdentifier(identifier)
        , mPlugin(plugin)
    {
        qCDebug(AKONADICORE_LOG) << " PLUGIN : identifier" << identifier;
    }

    QObject *plugin() const
    {
        if (mPlugin) {
            return mPlugin;
        }

        QObject *object = PluginLoader::self()->createForName(mIdentifier);
        if (!object) {
            qCWarning(AKONADICORE_LOG) << "ItemSerializerPluginLoader: "
                                       << "plugin" << mIdentifier << "is not valid!";

            // we try to use the default in that case
            mPlugin = s_defaultItemSerializerPlugin;
        }

        mPlugin = object;
        if (!qobject_cast<ItemSerializerPlugin *>(mPlugin)) {
            qCWarning(AKONADICORE_LOG) << "ItemSerializerPluginLoader: "
                                       << "plugin" << mIdentifier << "doesn't provide interface ItemSerializerPlugin!";

            // we try to use the default in that case
            mPlugin = s_defaultItemSerializerPlugin;
        }

        Q_ASSERT(mPlugin);

        return mPlugin;
    }

    const char *pluginClassName() const
    {
        return plugin()->metaObject()->className();
    }

    QString identifier() const
    {
        return mIdentifier;
    }

    bool operator<(const PluginEntry &other) const
    {
        return mIdentifier < other.mIdentifier;
    }

    bool operator<(const QString &identifier) const
    {
        return mIdentifier < identifier;
    }

private:
    QString mIdentifier;
    mutable QObject *mPlugin;
};

class MimeTypeEntry
{
public:
    explicit MimeTypeEntry(const QString &mimeType)
        : m_mimeType(mimeType)
    {
    }

    QString type() const
    {
        return m_mimeType;
    }

    void add(const QByteArray &class_, const PluginEntry &entry)
    {
        m_pluginsByMetaTypeId.clear(); // iterators will be invalidated by next line
        m_plugins.insert(class_, entry);
    }

    const PluginEntry *plugin(const QByteArray &class_) const
    {
        const QHash<QByteArray, PluginEntry>::const_iterator it = m_plugins.find(class_);
        return it == m_plugins.end() ? nullptr : it.operator->();
    }

    const PluginEntry *defaultPlugin() const
    {
        // 1. If there's an explicit default plugin, use that one:
        if (const PluginEntry *pe = plugin(DEFAULT_NAME)) {
            return pe;
        }

        // 2. Otherwise, look through the already instantiated plugins,
        //    and return one of them (preferably not the legacy one):
        bool sawZero = false;
        for (QMap<int, QHash<QByteArray, PluginEntry>::const_iterator>::const_iterator it = m_pluginsByMetaTypeId.constBegin(),
                                                                                       end = m_pluginsByMetaTypeId.constEnd();
             it != end;
             ++it) {
            if (it.key() == 0) {
                sawZero = true;
            } else if (*it != m_plugins.end()) {
                return it->operator->();
            }
        }

        // 3. Otherwise, look through the whole list (again, preferably not the legacy one):
        for (QHash<QByteArray, PluginEntry>::const_iterator it = m_plugins.constBegin(), end = m_plugins.constEnd(); it != end; ++it) {
            if (it.key() == LEGACY_NAME) {
                sawZero = true;
            } else {
                return it.operator->();
            }
        }

        // 4. take the legacy one:
        if (sawZero) {
            return plugin(0);
        }
        return nullptr;
    }

    const PluginEntry *plugin(int metaTypeId) const
    {
        const QMap<int, QHash<QByteArray, PluginEntry>::const_iterator> &c_pluginsByMetaTypeId = m_pluginsByMetaTypeId;
        QMap<int, QHash<QByteArray, PluginEntry>::const_iterator>::const_iterator it = c_pluginsByMetaTypeId.find(metaTypeId);
        if (it == c_pluginsByMetaTypeId.end()) {
            it = QMap<int, QHash<QByteArray, PluginEntry>::const_iterator>::const_iterator(
                m_pluginsByMetaTypeId.insert(metaTypeId, m_plugins.find(metaTypeId ? QMetaType(metaTypeId).name() : LEGACY_NAME)));
        }
        return *it == m_plugins.end() ? nullptr : it->operator->();
    }

    const PluginEntry *plugin(const QList<int> &metaTypeIds, int &chosen) const
    {
        bool sawZero = false;
        for (QList<int>::const_iterator it = metaTypeIds.begin(), end = metaTypeIds.end(); it != end; ++it) {
            if (*it == 0) {
                sawZero = true; // skip the legacy type and see if we can find something else first
            } else if (const PluginEntry *const entry = plugin(*it)) {
                chosen = *it;
                return entry;
            }
        }
        if (sawZero) {
            chosen = 0;
            return plugin(0);
        }
        return nullptr;
    }

private:
    QString m_mimeType;
    QHash<QByteArray /* class */, PluginEntry> m_plugins;
    mutable QMap<int, QHash<QByteArray, PluginEntry>::const_iterator> m_pluginsByMetaTypeId;
};

class PluginRegistry
{
public:
    PluginRegistry()
        : mDefaultPlugin(PluginEntry(QStringLiteral("application/octet-stream@QByteArray"), s_defaultItemSerializerPlugin))
        , mOverridePlugin(nullptr)
    {
        const PluginLoader *pl = PluginLoader::self();
        if (!pl) {
            qCWarning(AKONADICORE_LOG) << "Cannot instantiate plugin loader!";
            return;
        }
        const QStringList names = pl->names();
        qCDebug(AKONADICORE_LOG) << "ItemSerializerPluginLoader: "
                                 << "found" << names.size() << "plugins.";
        QMap<QString, MimeTypeEntry> map;
        QRegularExpression rx(QRegularExpression::anchoredPattern(QStringLiteral("(.+)@(.+)")));
        QMimeDatabase mimeDb;
        for (const QString &name : names) {
            QRegularExpressionMatch match = rx.match(name);
            if (match.hasMatch()) {
                const QMimeType mime = mimeDb.mimeTypeForName(match.captured(1));
                if (mime.isValid()) {
                    const QString mimeType = mime.name();
                    const QByteArray classType = match.captured(2).toLatin1();
                    QMap<QString, MimeTypeEntry>::iterator it = map.find(mimeType);
                    if (it == map.end()) {
                        it = map.insert(mimeType, MimeTypeEntry(mimeType));
                    }
                    it->add(classType, PluginEntry(name));
                }
            } else {
                qCDebug(AKONADICORE_LOG) << "ItemSerializerPluginLoader: "
                                         << "name" << name << "doesn't look like mimetype@classtype";
            }
        }
        const QString APPLICATION_OCTETSTREAM = QLatin1StringView(_APPLICATION_OCTETSTREAM);
        QMap<QString, MimeTypeEntry>::iterator it = map.find(APPLICATION_OCTETSTREAM);
        if (it == map.end()) {
            it = map.insert(APPLICATION_OCTETSTREAM, MimeTypeEntry(APPLICATION_OCTETSTREAM));
        }
        it->add("QByteArray", mDefaultPlugin);
        it->add(LEGACY_NAME, mDefaultPlugin);
        const int size = map.size();
        allMimeTypes.reserve(size);
        std::copy(map.begin(), map.end(), std::back_inserter(allMimeTypes));
    }

    QObject *findBestMatch(const QString &type, const QList<int> &metaTypeId, TypePluginLoader::Options opt)
    {
        if (QObject *const plugin = findBestMatch(type, metaTypeId)) {
            {
                if ((opt & TypePluginLoader::NoDefault) && plugin == mDefaultPlugin.plugin()) {
                    return nullptr;
                }
                return plugin;
            }
        }
        return nullptr;
    }

    QObject *findBestMatch(const QString &type, const QList<int> &metaTypeIds)
    {
        if (mOverridePlugin) {
            return mOverridePlugin;
        }
        if (QObject *const plugin = cacheLookup(type, metaTypeIds)) {
            // plugin cached, so let's take that one
            return plugin;
        }
        int chosen = -1;
        QObject *const plugin = findBestMatchImpl(type, metaTypeIds, chosen);
        if (metaTypeIds.empty()) {
            if (plugin) {
                cachedDefaultPlugins[type] = plugin;
            }
        }
        if (chosen >= 0) {
            cachedPlugins[type][chosen] = plugin;
        }
        return plugin;
    }

    void overrideDefaultPlugin(QObject *p)
    {
        mOverridePlugin = p;
    }

private:
    // Returns plugin matches for a mimetype in best->worst order, in terms of mimetype specificity
    void findSuitablePlugins(QMimeType mimeType, QSet<QMimeType> &checkedMimeTypes, QList<int> &matchingIndexes, const QMimeDatabase &mimeDb) const
    {
        // Avoid adding duplicates to our matchingIndexes
        if (checkedMimeTypes.contains(mimeType)) {
            return;
        }

        checkedMimeTypes.insert(mimeType);

        // Check each of the mimetypes we have plugins for to find a match
        for (int i = 0, end = allMimeTypes.size(); i < end; ++i) {
            const QMimeType pluginMimeType = mimeDb.mimeTypeForName(allMimeTypes[i].type()); // Convert from Akonadi::MimeTypeEntry
            if (!pluginMimeType.isValid() || pluginMimeType != mimeType) {
                continue;
            }

            matchingIndexes.append(i); // We found a match! This mimetype is supported by one of our plugins
        }

        const auto parentTypes = mimeType.parentMimeTypes();

        // Recursively move up the mimetype tree (checking less specific mimetypes)
        for (const auto &parent : parentTypes) {
            QMimeType parentType = mimeDb.mimeTypeForName(parent);
            findSuitablePlugins(parentType, checkedMimeTypes, matchingIndexes, mimeDb);
        }
    };

    QObject *findBestMatchImpl(const QString &type, const QList<int> &metaTypeIds, int &chosen) const
    {
        const QMimeDatabase mimeDb;
        const QMimeType mimeType = mimeDb.mimeTypeForName(type);
        if (!mimeType.isValid()) {
            qCWarning(AKONADICORE_LOG) << "Invalid mimetype requested:" << type;
            return mDefaultPlugin.plugin();
        }

        QSet<QMimeType> checkedMimeTypes;
        QList<int> matchingIndexes;

        findSuitablePlugins(mimeType, checkedMimeTypes, matchingIndexes, mimeDb);

        // Ask each one in turn if it can handle any of the metaTypeIds:
        //       qCDebug(AKONADICORE_LOG) << "Looking for " << format( type, metaTypeIds );
        for (QList<int>::const_iterator it = matchingIndexes.constBegin(), end = matchingIndexes.constEnd(); it != end; ++it) {
            //         qCDebug(AKONADICORE_LOG) << "  Considering serializer plugin for type" << allMimeTypes[matchingIndexes[*it]].type()
            // //                  << "as the closest match";
            const MimeTypeEntry &mt = allMimeTypes[*it];
            if (metaTypeIds.empty()) {
                if (const PluginEntry *const entry = mt.defaultPlugin()) {
                    //             qCDebug(AKONADICORE_LOG) << "    -> got " << entry->pluginClassName() << " and am happy with it.";
                    // FIXME ? in qt5 we show "application/octet-stream" first so if will use default plugin. Exclude it until we look at all mimetype and use
                    // default at the end if necessary
                    if (allMimeTypes[*it].type() != QLatin1StringView("application/octet-stream")) {
                        return entry->plugin();
                    }
                } else {
                    //             qCDebug(AKONADICORE_LOG) << "    -> no default plugin for this mime type, trying next";
                }
            } else if (const PluginEntry *const entry = mt.plugin(metaTypeIds, chosen)) {
                //           qCDebug(AKONADICORE_LOG) << "    -> got " << entry->pluginClassName() << " and am happy with it.";
                return entry->plugin();
            } else {
                //           qCDebug(AKONADICORE_LOG) << "   -> can't handle any of the types, trying next";
            }
        }

        //       qCDebug(AKONADICORE_LOG) << "  No further candidates, using default plugin";
        // no luck? Use the default plugin
        return mDefaultPlugin.plugin();
    }

    std::vector<MimeTypeEntry> allMimeTypes; // All the mimetypes that we have plugins for
    QHash<QString, QMap<int, QObject *>> cachedPlugins;
    QHash<QString, QObject *> cachedDefaultPlugins;

    // ### cache NULLs, too
    QObject *cacheLookup(const QString &mimeType, const QList<int> &metaTypeIds) const
    {
        if (metaTypeIds.empty()) {
            const QHash<QString, QObject *>::const_iterator hit = cachedDefaultPlugins.find(mimeType);
            if (hit != cachedDefaultPlugins.end()) {
                return *hit;
            }
        }

        const QHash<QString, QMap<int, QObject *>>::const_iterator hit = cachedPlugins.find(mimeType);
        if (hit == cachedPlugins.end()) {
            return nullptr;
        }
        bool sawZero = false;
        for (QList<int>::const_iterator it = metaTypeIds.begin(), end = metaTypeIds.end(); it != end; ++it) {
            if (*it == 0) {
                sawZero = true; // skip the legacy type and see if we can find something else first
            } else if (QObject *const o = hit->value(*it)) {
                return o;
            }
        }
        if (sawZero) {
            return hit->value(0);
        }
        return nullptr;
    }

private:
    PluginEntry mDefaultPlugin;
    QObject *mOverridePlugin;
};

Q_GLOBAL_STATIC(PluginRegistry, s_pluginRegistry) // NOLINT(readability-redundant-member-init)

QObject *TypePluginLoader::objectForMimeTypeAndClass(const QString &mimetype, const QList<int> &metaTypeIds, Options opt)
{
    return s_pluginRegistry->findBestMatch(mimetype, metaTypeIds, opt);
}

QObject *TypePluginLoader::defaultObjectForMimeType(const QString &mimetype)
{
    return objectForMimeTypeAndClass(mimetype, QList<int>());
}

ItemSerializerPlugin *TypePluginLoader::pluginForMimeTypeAndClass(const QString &mimetype, const QList<int> &metaTypeIds, Options opt)
{
    return qobject_cast<ItemSerializerPlugin *>(objectForMimeTypeAndClass(mimetype, metaTypeIds, opt));
}

ItemSerializerPlugin *TypePluginLoader::defaultPluginForMimeType(const QString &mimetype)
{
    ItemSerializerPlugin *plugin = qobject_cast<ItemSerializerPlugin *>(defaultObjectForMimeType(mimetype));
    Q_ASSERT(plugin);
    return plugin;
}

void TypePluginLoader::overridePluginLookup(QObject *p)
{
    s_pluginRegistry->overrideDefaultPlugin(p);
}

} // namespace Akonadi
