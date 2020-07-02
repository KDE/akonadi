/*  -*- c++ -*-
    SPDX-FileCopyrightText: 2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pluginloader_p.h"
#include "akonadicore_debug.h"
#include <private/standarddirs_p.h>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KConfig>
#include <QDir>
#include <QStandardPaths>
#include <QPluginLoader>

using namespace Akonadi;

PluginMetaData::PluginMetaData()
    : loaded(false)
{
}

PluginMetaData::PluginMetaData(const QString &lib, const QString &name, const QString &comment, const QString &cname)
    : library(lib)
    , nameLabel(name)
    , descriptionLabel(comment)
    , className(cname)
    , loaded(false)
{
}

PluginLoader *PluginLoader::mSelf = nullptr;

PluginLoader::PluginLoader()
{
    scan();
}

PluginLoader::~PluginLoader()
{
    qDeleteAll(mPluginLoaders);
    mPluginLoaders.clear();
}

PluginLoader *PluginLoader::self()
{
    if (!mSelf) {
        mSelf = new PluginLoader();
    }

    return mSelf;
}

QStringList PluginLoader::names() const
{
    return mPluginInfos.keys();
}

QObject *PluginLoader::createForName(const QString &name)
{
    if (!mPluginInfos.contains(name)) {
        qCWarning(AKONADICORE_LOG) << "plugin name \"" << name << "\" is unknown to the plugin loader.";
        return nullptr;
    }

    PluginMetaData &info = mPluginInfos[name];

    //First try to load it staticly
    const auto instances = QPluginLoader::staticInstances();
    for (auto *plugin : instances) {
        if (QLatin1String(plugin->metaObject()->className()) == info.className) {
            info.loaded = true;
            return plugin;
        }
    }

    if (!info.loaded) {
        QPluginLoader *loader = new QPluginLoader(info.library);
        if (loader->fileName().isEmpty()) {
            qCWarning(AKONADICORE_LOG) << "Error loading" << info.library << ":" << loader->errorString();
            delete loader;
            return nullptr;
        }

        mPluginLoaders.insert(name, loader);
        info.loaded = true;
    }

    QPluginLoader *loader = mPluginLoaders.value(name);
    Q_ASSERT(loader);

    QObject *object = loader->instance();
    if (!object) {
        qCWarning(AKONADICORE_LOG) << "unable to load plugin" << info.library << "for plugin name" << name << ".";
        qCWarning(AKONADICORE_LOG) << "Error was:\"" << loader->errorString() << "\".";
        return nullptr;
    }

    return object;
}

PluginMetaData PluginLoader::infoForName(const QString &name) const
{
    return mPluginInfos.value(name, PluginMetaData());
}

void PluginLoader::scan()
{
    const auto dirs = StandardDirs::locateAllResourceDirs(QStringLiteral("akonadi/plugins/serializer/"));
    for (const QString &dir : dirs) {
        const QStringList fileNames = QDir(dir).entryList(QStringList() << QStringLiteral("*.desktop"));
        for (const QString &file : fileNames) {
            const QString entry = dir + QLatin1Char('/') + file;
            KConfig config(entry, KConfig::SimpleConfig);
            if (config.hasGroup("Misc") && config.hasGroup("Plugin")) {
                KConfigGroup group(&config, "Plugin");

                const QString type = group.readEntry("Type").toLower();
                if (type.isEmpty()) {
                    qCWarning(AKONADICORE_LOG) << "missing or empty [Plugin]Type value in" << entry << "- skipping";
                    continue;
                }

                // read Class entry as a list so that types like QPair<A,B> are
                // properly escaped and don't end up being split into QPair<A
                // and B>.
                const QStringList classes = group.readXdgListEntry("X-Akonadi-Class");
                if (classes.isEmpty()) {
                    qCWarning(AKONADICORE_LOG) << "missing or empty [Plugin]X-Akonadi-Class value in" << entry << "- skipping";
                    continue;
                }

                const QString library = group.readEntry("X-KDE-Library");
                if (library.isEmpty()) {
                    qCWarning(AKONADICORE_LOG) << "missing or empty [Plugin]X-KDE-Library value in" << entry << "- skipping";
                    continue;
                }

                KConfigGroup group2(&config, "Misc");

                QString name = group2.readEntry("Name");
                if (name.isEmpty()) {
                    qCWarning(AKONADICORE_LOG) << "missing or empty [Misc]Name value in \"" << entry << "\" - inserting default name";
                    name = i18n("Unnamed plugin");
                }

                QString comment = group2.readEntry("Comment");
                if (comment.isEmpty()) {
                    qCWarning(AKONADICORE_LOG) << "missing or empty [Misc]Comment value in \"" << entry << "\" - inserting default name";
                    comment = i18n("No description available");
                }

                QString cname      = group.readEntry("X-KDE-ClassName");
                if (cname.isEmpty()) {
                    qCWarning(AKONADICORE_LOG) << "missing or empty X-KDE-ClassName value in \"" << entry << "\"";
                }

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                const QStringList mimeTypes = type.split(QLatin1Char(','), QString::SkipEmptyParts);
#else
                const QStringList mimeTypes = type.split(QLatin1Char(','), Qt::SkipEmptyParts);
#endif

                qCDebug(AKONADICORE_LOG) << "registering Desktop file" << entry << "for" << mimeTypes << '@' << classes;
                for (const QString &mimeType : mimeTypes) {
                    for (const QString &classType : classes) {
                        mPluginInfos.insert(mimeType + QLatin1Char('@') + classType, PluginMetaData(library, name, comment, cname));
                    }
                }

            } else {
                qCWarning(AKONADICORE_LOG) << "Desktop file \"" << entry << "\" doesn't seem to describe a plugin " << "(misses Misc and/or Plugin group)";
            }
        }
    }
}
