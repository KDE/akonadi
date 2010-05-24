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

class QString;

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
 * Currently it returns an ItemSerializerPlugin object to keep backwards compatibility, in KDE5
 * this should be changed to return a TypePlugin object, which provides a ItemSerializerInterface,
 * a DifferenceAlgorithmInterface etc.
 * In the meantime, the interfaces have to be requested via qobject_cast from the ItemSerializerPlugin
 * object.
 */
namespace TypePluginLoader {

/**
 * Returns the item serializer plugin that matches the given @p mimetype.
 */
ItemSerializerPlugin* pluginForMimeType( const QString &mimetype );

}

}

#endif
