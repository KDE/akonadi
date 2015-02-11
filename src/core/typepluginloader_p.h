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

#ifndef AKONADI_TYPEPLUGINLOADER_P_H
#define AKONADI_TYPEPLUGINLOADER_P_H

#include <QtCore/qglobal.h>
#include "akonadicore_export.h"

class QObject;
class QString;
template <typename T>
class QVector;

namespace Akonadi {
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
namespace TypePluginLoader {

enum Option {
    NoOptions,
    NoDefault = 1,

    _LastOption,
    OptionMask = 2 * _LastOption - 1
};
Q_DECLARE_FLAGS(Options, Option)

#if 0
/**
 * Returns the legacy (pre-KDE-4.6) item serializer plugin that matches the given @p mimetype.
 */
ItemSerializerPlugin *legacyPluginForMimeType(const QString &mimetype);
#endif

/**
 * Returns the default item serializer plugin that matches the given @p mimetype.
 */
AKONADICORE_EXPORT ItemSerializerPlugin *defaultPluginForMimeType(const QString &mimetype);

/**
 * Returns the item serializer plugin that matches the given
 * @p mimetype, and any of the classes described by @p metaTypeIds.
 */
AKONADICORE_EXPORT ItemSerializerPlugin *pluginForMimeTypeAndClass(const QString &mimetype, const QVector<int> &metaTypeIds, Options options = NoOptions);

#if 0
/**
 * Returns the legacy (pre-KDE-4.6) type plugin object that matches the given @p mimetype.
 */
QObject *legacyObjectForMimeType(const QString &mimetype);
#endif

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
AKONADICORE_EXPORT void Q_DECL_OVERRIDEPluginLookup(QObject *plugin);

}

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Akonadi::TypePluginLoader::Options)

#endif
