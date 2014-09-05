/*
 Copyright (c) 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

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

#ifndef AKONADI_TRASHSETTINGS_H
#define AKONADI_TRASHSETTINGS_H

#include "akonadicore_export.h"
#include "collection.h"

class QString;

namespace Akonadi {

/**
  * @short Global Trash-related Settings
  *
  * All settings concerning the trashhandling should go here.
  *
  * @author Christian Mollekopf <chrigi_1@fastmail.fm>
  * @since 4.8
  */
//TODO setting for time before items are deleted by janitor agent
namespace TrashSettings {
/**
 * Set the trash collection for the given @p resource which is then used by the TrashJob
 */
AKONADICORE_EXPORT void setTrashCollection(const QString &resource, const Collection &collection);
/**
 * Get the trash collection for the given @p resource
 */
AKONADICORE_EXPORT Collection getTrashCollection(const QString &resource);
}

}

#endif
