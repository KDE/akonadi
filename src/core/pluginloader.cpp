/*  -*- c++ -*-
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "pluginloader_p.h"
#include "akonadicore_debug.h"
#include <kconfiggroup.h>
#include <KLocalizedString>
#include <KPluginLoader>
#include <KConfig>
#include <QDir>
#include <QStandardPaths>

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

PluginLoader *PluginLoader::mSelf = Q_NULLPTR;

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
        qCWarning(AKONADICORE_LOG) << "plugin name \"" << name << "\" is unknown to the plugin loader." << endl;
        return 0;
    }

    PluginMetaData &info = mPluginInfos[name];

    //First try to load it staticly
    foreach (QObject *plugin, QPluginLoader::staticInstances()) {
        if (QLatin1String(plugin->metaObject()->className()) == info.className) {
            info.loaded = true;
            return plugin;
            break;
        }
    }

    if (!info.loaded) {
        QPluginLoader *loader = new QPluginLoader(info.library);
        if (loader->fileName().isEmpty()) {
            qCWarning(AKONADICORE_LOG) << loader->errorString();
            delete loader;
            return 0;
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
        return 0;
    }

    return object;
}

PluginMetaData PluginLoader::infoForName(const QString &name) const
{
    if (!mPluginInfos.contains(name)) {
        return PluginMetaData();
    }

    return mPluginInfos.value(name);
}

void PluginLoader::scan()
{
    const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("akonadi/plugins/serializer/"), QStandardPaths::LocateDirectory);
    Q_FOREACH (const QString &dir, dirs) {
        const QStringList fileNames = QDir(dir).entryList(QStringList() << QStringLiteral("*.desktop"));
        Q_FOREACH (const QString &file, fileNames) {
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
                    qCWarning(AKONADICORE_LOG) << "missing or empty [Misc]Name value in \"" << entry << "\" - inserting default name" << endl;
                    name = i18n("Unnamed plugin");
                }

                QString comment = group2.readEntry("Comment");
                if (comment.isEmpty()) {
                    qCWarning(AKONADICORE_LOG) << "missing or empty [Misc]Comment value in \"" << entry << "\" - inserting default name" << endl;
                    comment = i18n("No description available");
                }

                QString cname      = group.readEntry("X-KDE-ClassName");
                if (cname.isEmpty()) {
                    qCWarning(AKONADICORE_LOG) << "missing or empty X-KDE-ClassName value in \"" << entry << "\"" << endl;
                }

                const QStringList mimeTypes = type.split(QLatin1Char(','), QString::SkipEmptyParts);

                qCDebug(AKONADICORE_LOG) << "registering Desktop file" << entry << "for" << mimeTypes << '@' << classes;
                Q_FOREACH (const QString &mimeType, mimeTypes) {
                    Q_FOREACH (const QString &classType, classes) {
                        mPluginInfos.insert(mimeType + QLatin1Char('@') + classType, PluginMetaData(library, name, comment, cname));
                    }
                }

            } else {
                qCWarning(AKONADICORE_LOG) << "Desktop file \"" << entry << "\" doesn't seem to describe a plugin " << "(misses Misc and/or Plugin group)" << endl;
            }
        }
    }
}
