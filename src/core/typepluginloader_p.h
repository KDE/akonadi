/*
    SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include <qglobal.h>

class QObject;
class QString;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
template<typename T> class QVector;
#else
template<typename T>
class QList;
#endif

namespace Akonadi
{
class ItemSerializerPlugin;

/**
 * @internal
 *
 * With KDE 4.6 we are on the way to change the ItemSerializer plugins into general TypePlugins
 * which provide several type specific actions, namely:
 *   @li Serializing/Deserializing of payload
 *   @li Comparing two payloads and reporting the differences
 *
 * To share the code of loading the plugins and finding the right plugin for a given mime type
 * the old code from ItemSerializer has been extracted into the pluginForMimeType() method
 * inside the TypePluginLoader namespace.
 */
namespace TypePluginLoader
{
enum Option {
    NoOptions,
    NoDefault = 1,

    _LastOption,
    OptionMask = 2 * _LastOption - 1
};
Q_DECLARE_FLAGS(Options, Option)

/**
 * Returns the default item serializer plugin that matches the given @p mimetype.
 */
AKONADICORE_EXPORT ItemSerializerPlugin *defaultPluginForMimeType(const QString &mimetype);

/**
 * Returns the item serializer plugin that matches the given
 * @p mimetype, and any of the classes described by @p metaTypeIds.
 */
AKONADICORE_EXPORT ItemSerializerPlugin *pluginForMimeTypeAndClass(const QString &mimetype, const QVector<int> &metaTypeIds, Options options = NoOptions);

/**
 * Returns the default type plugin object that matches the given @p mimetype.
 */
AKONADICORE_EXPORT QObject *defaultObjectForMimeType(const QString &mimetype);

/**
 * Returns the type plugin object that matches the given @p mimetype,
 * and any of the classes described by @p metaTypeIds.
 */
AKONADICORE_EXPORT QObject *objectForMimeTypeAndClass(const QString &mimetype, const QVector<int> &metaTypeIds, Options options = NoOptions);

/**
 * Override the plugin-lookup with @p plugin.
 *
 * After calling this each lookup will always return @p plugin.
 * This is useful to inject a special plugin for testing purposes.
 * To reset the plugin, set to 0.
 *
 * @since 4.12
 */
AKONADICORE_EXPORT void overridePluginLookup(QObject *plugin);

}

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Akonadi::TypePluginLoader::Options)

