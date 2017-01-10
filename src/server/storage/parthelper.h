/***************************************************************************
 *   Copyright (C) 2009 by Andras Mantia <amantia@kde.org>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef PARTHELPER_H
#define PARTHELPER_H

#include <QtGlobal>
#include "entities.h"
#include "../exception.h"

class QString;
class QVariant;

namespace Akonadi
{
namespace Server
{

AKONADI_EXCEPTION_MAKE_INSTANCE(PartHelperException);

/**
 * Helper methods that store data in a file instead of the database.
 *
 * @author Andras Mantia <amantia@kde.org>
 *
 * @todo Use exceptions for error handling in all these methods. Requires that all callers
 * can handle that first though.
 */
namespace PartHelper
{
/**
 * Update payload of an existing part @p part to @p data and size @p dataSize.
 * Automatically decides whether or not the data should be stored in the database
 * or the file system.
 * @throw PartHelperException if file operations failed
 */
void update(Part *part, const QByteArray &data, qint64 dataSize);

/**
 * Adds a new part to the database and if necessary to the filesystem.
 * @p part must not be in the database yet (ie. valid() == false) and must have
 * a data size set.
 */
bool insert(Part *part, qint64 *insertId = nullptr);

/** Deletes @p part from the database and also removes existing filesystem data if needed. */
bool remove(Part *part);
/** Deletes all parts which match the given constraint, including all corresponding filesystem data. */
bool remove(const QString &column, const QVariant &value);

/** Returns the payload data. */
QByteArray translateData(const QByteArray &data, bool isExternal);
/** Convenience overload of the above. */
QByteArray translateData(const Part &part);

/** Truncate the payload of @p part and update filesystem/database accordingly.
 *  This is more efficient than using update since it does not require the data to be loaded.
 */
bool truncate(Part &part);

/** Verifies and if necessary fixes the external reference of this part. */
bool verify(Part &part);

} // namespace PartHelper

} // namespace Server
} // namespace Akonadi

#endif
