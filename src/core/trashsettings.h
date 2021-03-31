/*
 SPDX-FileCopyrightText: 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "collection.h"

class QString;

namespace Akonadi
{
/**
 * @short Global Trash-related Settings
 *
 * All settings concerning the trashhandling should go here.
 *
 * @author Christian Mollekopf <chrigi_1@fastmail.fm>
 * @since 4.8
 */
// TODO setting for time before items are deleted by janitor agent
namespace TrashSettings
{
/**
 * Set the trash collection for the given @p resource which is then used by the TrashJob
 */
AKONADICORE_EXPORT void setTrashCollection(const QString &resource, const Collection &collection);
/**
 * Get the trash collection for the given @p resource
 */
Q_REQUIRED_RESULT AKONADICORE_EXPORT Collection getTrashCollection(const QString &resource);
}

}

