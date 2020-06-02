/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
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

#include "typepluginloader_p.h"

#include "item.h"
#include "itemserializer_p.h"
#include "itemserializerplugin.h"

#include "akonadicore_debug.h"

// Qt
#include <QHash>
#include <QString>
#include <QByteArray>
#include <QStringList>
#include <QMimeDatabase>
#include <QMimeType>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>

// temporary
#include "pluginloader_p.h"

#include <vector>
#include <cassert>

static const char LEGACY_NAME[] = "legacy";
static const char DEFAULT_NAME[] = "default";
static const char _APPLICATION_OCTETSTREAM[] = "application/octet-stream";

namespace Akonadi
{

Q_GLOBAL_STATIC(DefaultItemSerializerPlugin, s_defaultItemSerializerPlugin)

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
                                       << "plugin" << mIdentifier
                                       << "doesn't provide interface ItemSerializerPlugin!";

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
    {}

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
        for (QMap<int, QHash<QByteArray, PluginEntry>::const_iterator>::const_iterator it = m_pluginsByMetaTypeId.constBegin(), end = m_pluginsByMetaTypeId.constEnd(); it != end; ++it) {
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
            it = QMap<int, QHash<QByteArray, PluginEntry>::const_iterator>::const_iterator(m_pluginsByMetaTypeId.insert(metaTypeId, m_plugins.find(metaTypeId ? QMetaType::typeName(metaTypeId) : LEGACY_NAME)));
        }
        return *it == m_plugins.end() ? nullptr : it->operator->();
    }

    const PluginEntry *plugin(const QVector<int> &metaTypeIds, int &chosen) const
    {
        bool sawZero = false;
        for (QVector<int>::const_iterator it = metaTypeIds.begin(), end = metaTypeIds.end(); it != end; ++it) {
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
    QHash<QByteArray/* class */, PluginEntry> m_plugins;
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
        QRegExp rx(QStringLiteral("(.+)@(.+)"));
        QMimeDatabase mimeDb;
        for (const QString &name : names) {
            if (rx.exactMatch(name)) {
                const QMimeType mime = mimeDb.mimeTypeForName(rx.cap(1));
                if (mime.isValid()) {
                    const QString mimeType = mime.name();
                    const QByteArray classType = rx.cap(2).toLatin1();
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
        const QString APPLICATION_OCTETSTREAM = QLatin1String(_APPLICATION_OCTETSTREAM);
        QMap<QString, MimeTypeEntry>::iterator it = map.find(APPLICATION_OCTETSTREAM);
        if (it == map.end()) {
            it = map.insert(APPLICATION_OCTETSTREAM, MimeTypeEntry(APPLICATION_OCTETSTREAM));
        }
        it->add("QByteArray", mDefaultPlugin);
        it->add(LEGACY_NAME,  mDefaultPlugin);
        const int size = map.size();
        allMimeTypes.reserve(size);
        std::copy(map.begin(), map.end(), std::back_inserter(allMimeTypes));
    }

    QObject *findBestMatch(const QString &type, const QVector<int> &metaTypeId, TypePluginLoader::Options opt)
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

    QObject *findBestMatch(const QString &type, const QVector<int> &metaTypeIds)
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
    QObject *findBestMatchImpl(const QString &type, const QVector<int> &metaTypeIds, int &chosen) const
    {
        const QMimeDatabase mimeDb;
        const QMimeType mimeType = mimeDb.mimeTypeForName(type);
        if (!mimeType.isValid()) {
            qCWarning(AKONADICORE_LOG) << "Invalid mimetype requested:" << type;
            return mDefaultPlugin.plugin();
        }

        // step 1: find all plugins that match at all
        QVector<int> matchingIndexes;
        for (int i = 0, end = allMimeTypes.size(); i < end; ++i) {
            if (mimeType.inherits(allMimeTypes[i].type())) {
                matchingIndexes.append(i);
            }
        }

        // step 2: if we have more than one match, find the most specific one using topological sort
        QVector<int> order;
        if (matchingIndexes.size() <= 1) {
            order.push_back(0);
        } else {
            boost::adjacency_list<> graph(matchingIndexes.size());
            for (int i = 0, end = matchingIndexes.size(); i != end; ++i) {
                const QMimeType mimeType = mimeDb.mimeTypeForName(allMimeTypes[matchingIndexes[i]].type());
                if (!mimeType.isValid()) {
                    continue;
                }
                for (int j = 0; j != end; ++j) {
                    if (i != j && mimeType.inherits(allMimeTypes[matchingIndexes[j]].type())) {
                        boost::add_edge(j, i, graph);
                    }
                }
            }

            order.reserve(matchingIndexes.size());
            try {
                boost::topological_sort(graph, std::back_inserter(order));
            } catch (const boost::not_a_dag &e) {
                qCWarning(AKONADICORE_LOG) << "Mimetype tree is not a DAG!";
                return mDefaultPlugin.plugin();
            }
        }

        // step 3: ask each one in turn if it can handle any of the metaTypeIds:
//       qCDebug(AKONADICORE_LOG) << "Looking for " << format( type, metaTypeIds );
        for (QVector<int>::const_iterator it = order.constBegin(), end = order.constEnd(); it != end; ++it) {
//         qCDebug(AKONADICORE_LOG) << "  Considering serializer plugin for type" << allMimeTypes[matchingIndexes[*it]].type()
// //                  << "as the closest match";
            const MimeTypeEntry &mt = allMimeTypes[matchingIndexes[*it]];
            if (metaTypeIds.empty()) {
                if (const PluginEntry *const entry = mt.defaultPlugin()) {
//             qCDebug(AKONADICORE_LOG) << "    -> got " << entry->pluginClassName() << " and am happy with it.";
                    //FIXME ? in qt5 we show "application/octet-stream" first so if will use default plugin. Exclude it until we look at all mimetype and use default at the end if necessary
                    if (allMimeTypes[matchingIndexes[*it]].type() != QLatin1String("application/octet-stream")) {
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

    std::vector<MimeTypeEntry> allMimeTypes;
    QHash<QString, QMap<int, QObject *> > cachedPlugins;
    QHash<QString, QObject *> cachedDefaultPlugins;

    // ### cache NULLs, too
    QObject *cacheLookup(const QString &mimeType, const QVector<int> &metaTypeIds) const
    {
        if (metaTypeIds.empty()) {
            const QHash<QString, QObject *>::const_iterator hit = cachedDefaultPlugins.find(mimeType);
            if (hit != cachedDefaultPlugins.end()) {
                return *hit;
            }
        }

        const QHash<QString, QMap<int, QObject *> >::const_iterator hit = cachedPlugins.find(mimeType);
        if (hit == cachedPlugins.end()) {
            return nullptr;
        }
        bool sawZero = false;
        for (QVector<int>::const_iterator it = metaTypeIds.begin(), end = metaTypeIds.end(); it != end; ++it) {
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

Q_GLOBAL_STATIC(PluginRegistry, s_pluginRegistry)

QObject *TypePluginLoader::objectForMimeTypeAndClass(const QString &mimetype, const QVector<int> &metaTypeIds, Options opt)
{
    return s_pluginRegistry->findBestMatch(mimetype, metaTypeIds, opt);
}

QObject *TypePluginLoader::defaultObjectForMimeType(const QString &mimetype)
{
    return objectForMimeTypeAndClass(mimetype, QVector<int>());
}

ItemSerializerPlugin *TypePluginLoader::pluginForMimeTypeAndClass(const QString &mimetype, const QVector<int> &metaTypeIds, Options opt)
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

}
